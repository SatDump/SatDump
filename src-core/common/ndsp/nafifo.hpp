#include "nafifo.h"

#include <functional>

class NaFiFo
{
private:
    struct nafifo_t *obj = nullptr;

    std::function<void *()> alloc_f_p;
    std::function<void(void *)> free_f_p;

    static void *alloc_f_c(void *tthis);
    static void free_f_c(void *, void *);

public:
    inline void init(int size, int typesize, std::function<void *()> alloc_f, std::function<void(void *)> free_f)
    {
        free();

        alloc_f_p = alloc_f;
        free_f_p = free_f;
        obj = nafifo_init(size, typesize, alloc_f_c, free_f_c, this);
    }

    inline void free()
    {
        if (obj != nullptr)
            nafifo_free(&obj);
    }

    inline int write()
    {
        return nafifo_write(obj);
    }

    inline int read()
    {
        return nafifo_read(obj);
    }

    inline void flush()
    {
        nafifo_flush(obj);
    }

    inline void stop()
    {
        nafifo_stop(obj);
    }

    inline void status_print()
    {
        nafifo_status_print(obj);
    }

    inline void *write_buf()
    {
        return obj->write_buf;
    }

    inline void *read_buf()
    {
        return obj->read_buf;
    }
};