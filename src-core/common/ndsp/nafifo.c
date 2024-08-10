#include "nafifo.h"
#include <stdio.h>

nafifo_t *nafifo_init(int size, int typesize, void *(*alloc_f)(void *), void (*free_f)(void *, void *), void *alloc_usr)
{
    nafifo_t *f = malloc(sizeof(nafifo_t));

    // Setup internal constants
    f->size = size;
    f->typesize = typesize;
    f->alloc_usr = alloc_usr;
    f->alloc_f = alloc_f;
    f->free_f = free_f;

    // Allocate buffers we'll be swapping around
    f->ptr_buffer = malloc(sizeof(void *) * size);
    for (int i = 0; i < size; i++)
        f->ptr_buffer[i] = f->alloc_f(f->alloc_usr);

    // Init logic variable for both 2 and 3+ pointer modes
    f->can_swap = 1;
    f->data_ready = 0;
    f->writepos = 0;
    f->readpos = size - 1;

    // Init initial user pointers
    f->write_buf = f->ptr_buffer[f->writepos];
    f->read_buf = f->ptr_buffer[f->readpos];

    // Allocate semaphores/mutexes
    f->write_sem = malloc(sizeof(sem_t));
    f->read_sem = malloc(sizeof(sem_t));
    f->wr_mtx = malloc(sizeof(pthread_mutex_t));
    f->rd_mtx = malloc(sizeof(pthread_mutex_t));

    sem_init(f->write_sem, 0, 0);
    sem_init(f->read_sem, 0, 0);
    pthread_mutex_init(f->wr_mtx, NULL);
    pthread_mutex_init(f->rd_mtx, NULL);

    // No, let's not exit yet...
    f->should_exit = 0;

    return f;
}

void nafifo_free(nafifo_t **f)
{
    if (*f == NULL)
        return; // Better be safe...

    // Free all allocated buffers
    for (int i = 0; i < (*f)->size; i++)
        (*f)->free_f((*f)->alloc_usr, (*f)->ptr_buffer[i]);
    free((*f)->ptr_buffer);

    // Free semaphores/mutexes
    free((*f)->write_sem);
    free((*f)->read_sem);
    free((*f)->wr_mtx);
    free((*f)->rd_mtx);

    // Set user pointers to null, indicating
    (*f)->write_buf = NULL;
    (*f)->read_buf = NULL;

    // Finally free our struct
    free(*f);

    *f = NULL;
}

int nafifo_write(nafifo_t *f)
{
    if (f->size == 2)
    {
        // Wait until we are able to swap
        pthread_mutex_lock(f->wr_mtx);
        if (!f->can_swap)
        {
        retry_2:
            pthread_mutex_unlock(f->wr_mtx);
            sem_wait(f->read_sem);
            if (f->should_exit)
                return 1;
            pthread_mutex_lock(f->wr_mtx);

            if (!f->can_swap)
                goto retry_2;
        }

        // Swap buffers
        f->readpos = 1 - f->readpos;
        f->read_buf = f->ptr_buffer[f->readpos];

        f->writepos = 1 - f->writepos;
        f->write_buf = f->ptr_buffer[f->writepos];

        // We can't swap anymore
        f->can_swap = 0;
        pthread_mutex_unlock(f->wr_mtx);

        // We now have a buffer to provide the reader
        pthread_mutex_lock(f->rd_mtx);
        f->data_ready = 1;
        pthread_mutex_unlock(f->rd_mtx);

        // Notify the read thread that we've updated data_ready
        sem_post(f->write_sem);
    }
    else
    {
    retry:
        if (f->should_exit)
            return 1;

        // Atempt to lock the write mutex
        pthread_mutex_lock(f->wr_mtx);

        // Calculate the new pointer position if we increment by one.
        int newwritepos = ((f->writepos + 1) % f->size);
        if (f->readpos == newwritepos)
        { // It should not conflict with the current read pointer. If it does, wait until the read is finished.
            pthread_mutex_unlock(f->wr_mtx);
            sem_wait(f->read_sem);
            goto retry;
        }

        // Increment write pointer
        f->writepos = newwritepos;
        f->write_buf = f->ptr_buffer[f->writepos];

        pthread_mutex_unlock(f->wr_mtx);

        // Notify read thread in case it was waiting
        sem_post(f->write_sem);
    }

    return 0;
}

int nafifo_read(nafifo_t *f)
{
    if (f->size == 2)
    {
        // Wait for data to be available. If it is, the pointers was already swapped.
        pthread_mutex_lock(f->rd_mtx);

        if (!f->data_ready)
        {
        retry_2:
            pthread_mutex_unlock(f->rd_mtx);
            if (f->should_exit)
                return 1;
            sem_wait(f->write_sem);
            if (f->should_exit)
                return 1;
            pthread_mutex_lock(f->rd_mtx);
            if (!f->data_ready)
                goto retry_2;
        }

        pthread_mutex_unlock(f->rd_mtx);
    }
    else
    {
    retry:
        if (f->should_exit)
            return 1;

        // Attempt to lock both the read and write mutex
        pthread_mutex_lock(f->rd_mtx);
        pthread_mutex_lock(f->wr_mtx);

        // Calculate new read pointer, which shouldn't conflict with the write pointer
        int newreadpos = ((f->readpos + 1) % f->size);
        if (f->writepos == newreadpos)
        { // If it does... Wait till the write is commited
            pthread_mutex_unlock(f->rd_mtx);
            pthread_mutex_unlock(f->wr_mtx);
            sem_wait(f->write_sem);
            goto retry;
        }

        // Increment read pointer
        f->readpos = newreadpos;
        f->read_buf = f->ptr_buffer[f->readpos];

        pthread_mutex_unlock(f->wr_mtx);
        // Do not unlock the read mutex until flush() is called
    }

    return 0;
}

void nafifo_flush(nafifo_t *f)
{
    if (f->size == 2)
    {
        // We've consumed the data
        pthread_mutex_lock(f->rd_mtx);
        f->data_ready = 0;
        pthread_mutex_unlock(f->rd_mtx);

        // ...and can swap
        pthread_mutex_lock(f->wr_mtx);
        f->can_swap = 1;
        pthread_mutex_unlock(f->wr_mtx);

        // Notify the write function in case it was waiting
        sem_post(f->read_sem);
    }
    else
    {
        // Unlock
        pthread_mutex_unlock(f->rd_mtx);

        // Notify the writer thread a new buffer is free
        sem_post(f->read_sem);
    }
}

void nafifo_stop(nafifo_t *f)
{
    // Take over both writer and reader, tell them to exit
    pthread_mutex_trylock(f->rd_mtx); // pthread_mutex_lock ?
    pthread_mutex_lock(f->wr_mtx);    // pthread_mutex_lock ?
    f->should_exit = 1;
    pthread_mutex_unlock(f->rd_mtx);
    pthread_mutex_unlock(f->wr_mtx);

    // Notify them to force them to exit now
    sem_post(f->read_sem);
    sem_post(f->write_sem);
}

void nafifo_status_print(nafifo_t *f)
{
    printf("FIFO, ReadPos %d, WritePos %d\n", f->readpos, f->writepos);
}
