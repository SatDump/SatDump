#include "nafifo.hpp"

void *NaFiFo::alloc_f_c(void *tthis)
{
    return ((NaFiFo *)tthis)->alloc_f_p();
}

void NaFiFo::free_f_c(void *tthis, void *ptr)
{
    ((NaFiFo *)tthis)->free_f_p(ptr);
}