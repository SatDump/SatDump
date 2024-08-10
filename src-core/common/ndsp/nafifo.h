#pragma once

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>

/*
(Relatively) simple generic Non-Allocating FIFO meant for DSP
and similar applications.

Unlike a standard fifo or queue which will require memmoves or
memcpys in order to read/write elements, this is meant to only
pass around pre-allocated pointers of which you can have as many
as you wish as long as it's at least 2.

This is similar to simply swapping 2 pointers as done in Ryzerth's
stream implementation, but in this instance handling more than one
allows more flexibility in cases where more than one buffer is
preferrable. When only 2 are used, the behavior is identical to
this original implementation, and the usage is also identical
except the internals behave more akin to a circular buffer,
swapping around the read/write buffers.

It is worth noting no concept of buffer size is present to indicate
a number of sample. This should be handled by the user via a custom
struct or similar.

Aang23 - 06/05/2024
*/

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct nafifo_t
    {
        int size;          // Size of the fifo, in elements
        int typesize;      // Size of each element (can be arrays!)
        void **ptr_buffer; // Buffer of pointers

        void *alloc_usr;                // Element allocator/deleter user data
        void *(*alloc_f)(void *);       // Element allocator
        void (*free_f)(void *, void *); // Element free-er

        int can_swap, data_ready; // Used in 2-pointer mode
        int writepos, readpos;    // Used when above 2 pointers are used

        void *write_buf; // User write buffer, see nafifo_write
        void *read_buf;  // User read buffer, see nafifo_read and nafifo_flush

        sem_t *write_sem, *read_sem;      // Read and write semaphore
        pthread_mutex_t *wr_mtx, *rd_mtx; // Read and write mutexes

        int should_exit; // Flag to indicate termination has been requested.
    } nafifo_t;

    /*
    Initialize the fifo.
    size : Size of the fifo in number of elements. Must be at least 2
    typesize : Size of each element (should be sizeof(int)) for example
    alloc_f : Constructor for fifo elements
    free_f : Destructor for fifo elements
    */
    nafifo_t *nafifo_init(int size, int typesize, void *(*alloc_f)(void *), void (*free_f)(void *, void *), void *alloc_usr);

    /*
    Frees the fifo back to an uninitialized state. Must be called
    before exit.
    */
    void nafifo_free(nafifo_t **f);

    /*
    To write into the fifo, changes should be applied to the
    write_buf pointer. Once said changes are done, this method
    should be called to "commit" the write.
    Returns 1 when stopped.
    */
    int nafifo_write(nafifo_t *f);

    /*
    Request a read buffer from the fifo. This should be called
    before reading from the read_buf pointer. It is then
    possible to read safely, until nafifo_flush is called.
    */
    int nafifo_read(nafifo_t *f);

    /*
    This should be called after nafifo_read once all
    read operations are finished to signal reading is
    finished.
    */
    void nafifo_flush(nafifo_t *f);

    /*
    Signals the fifo it should terminate all blocking
    functions and return an exit flag (1).
    Once stopped, this cannot be re-used without free-ing and
    re-allocation at least with the current implementation.
    */
    void nafifo_stop(nafifo_t *f);

    /*
    Debug function, prints the current readpos/writepos
    into the console.
    */
    void nafifo_status_print(nafifo_t *f);

#ifdef __cplusplus
}
#endif