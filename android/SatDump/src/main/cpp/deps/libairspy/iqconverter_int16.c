/*
Copyright (C) 2014, Youssef Touil <youssef@airspy.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "iqconverter_int16.h"
#include <stdlib.h>
#include <string.h>

#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
  #include <malloc.h>
  #define _aligned_malloc __mingw_aligned_malloc
  #define _aligned_free  __mingw_aligned_free
  #define _inline inline
#elif defined(__APPLE__)
  #include <malloc/malloc.h>
  #define _aligned_malloc(size, alignment) malloc(size)
  #define _aligned_free(mem) free(mem)
  #define _inline inline
#elif defined(__FreeBSD__)
  #define _inline inline
  #define _aligned_free(mem) free(mem)
void * _aligned_malloc(size_t size, size_t alignment);
#elif defined(__GNUC__) && !defined(__MINGW64_VERSION_MAJOR)
  #include <malloc.h>
  #define _aligned_malloc(size, alignment) memalign(alignment, size)
  #define _aligned_free(mem) free(mem)
  #define _inline inline
#endif

#define SIZE_FACTOR 16
#define DEFAULT_ALIGNMENT 16

iqconverter_int16_t *iqconverter_int16_create(const int16_t *hb_kernel, int len)
{
	int i;
	size_t buffer_size;
	iqconverter_int16_t *cnv = (iqconverter_int16_t *) _aligned_malloc(sizeof(iqconverter_int16_t), DEFAULT_ALIGNMENT);

	cnv->len = len / 2 + 1;

	buffer_size = cnv->len * sizeof(int32_t);

	cnv->fir_kernel = (int32_t *) _aligned_malloc(buffer_size, DEFAULT_ALIGNMENT);
	cnv->fir_queue = (int32_t *) _aligned_malloc(buffer_size * SIZE_FACTOR, DEFAULT_ALIGNMENT);
	cnv->delay_line = (int16_t *) _aligned_malloc(buffer_size / 4, DEFAULT_ALIGNMENT);

	iqconverter_int16_reset(cnv);

	for (i = 0; i < cnv->len; i++)
	{
		cnv->fir_kernel[i] = hb_kernel[i * 2];
	}

	return cnv;
}

void iqconverter_int16_free(iqconverter_int16_t *cnv)
{
	_aligned_free(cnv->fir_kernel);
	_aligned_free(cnv->fir_queue);
	_aligned_free(cnv->delay_line);
	_aligned_free(cnv);
}

void iqconverter_int16_reset(iqconverter_int16_t *cnv)
{
	cnv->fir_index = 0;
	cnv->delay_index = 0;
	cnv->old_x = 0;
	cnv->old_y = 0;
	cnv->old_e = 0;
	memset(cnv->delay_line, 0, cnv->len * sizeof(int16_t) / 4);
	memset(cnv->fir_queue, 0, cnv->len * sizeof(int16_t) * SIZE_FACTOR);
}

static void fir_interleaved(iqconverter_int16_t *cnv, int16_t *samples, int len)
{
	int i;
	int j;
	int fir_index;
	int fir_len;
	int32_t *queue;
	int32_t acc;

	fir_len = cnv->len;
	fir_index = cnv->fir_index;

	for (i = 0; i < len; i += 2)
	{
		queue = cnv->fir_queue + fir_index;

		queue[0] = samples[i];

		acc = 0;
		
		// Auto vectorization works on VS2012, VS2013 and GCC
		for (j = 0; j < fir_len; j++)
		{
			acc += cnv->fir_kernel[j] * queue[j];
		}

		if (--fir_index < 0)
		{
			fir_index = cnv->len * (SIZE_FACTOR - 1);
			memcpy(cnv->fir_queue + fir_index + 1, cnv->fir_queue, (cnv->len - 1) * sizeof(int32_t));
		}

		samples[i] = acc >> 15;
	}

	cnv->fir_index = fir_index;
}

static void delay_interleaved(iqconverter_int16_t *cnv, int16_t *samples, int len)
{
	int i;
	int index;
	int half_len;
	int16_t res;
	
	half_len = cnv->len >> 1;
	index = cnv->delay_index;

	for (i = 0; i < len; i += 2)
	{
		res = cnv->delay_line[index];
		cnv->delay_line[index] = samples[i];
		samples[i] = res;

		if (++index >= half_len)
		{
			index = 0;
		}
	}
	
	cnv->delay_index = index;
}

static void remove_dc(iqconverter_int16_t *cnv, int16_t *samples, int len)
{
	int i;
	int32_t u, old_e;
	int16_t x, y, w, s, old_x, old_y;

	old_x = cnv->old_x;
	old_y = cnv->old_y;
	old_e = cnv->old_e;

	for (i = 0; i < len; i++)
	{
		x = samples[i];
		w = x - old_x;
		u = old_e + (int32_t) old_y * 32100;
		s = u >> 15;
		y = w + s;
		old_e = u - (s << 15);
		old_x = x;
		old_y = y;		
		samples[i] = y;
	}

	cnv->old_x = old_x;
	cnv->old_y = old_y;
	cnv->old_e = old_e;
}

static void translate_fs_4(iqconverter_int16_t *cnv, int16_t *samples, int len)
{
	int i;

	for (i = 0; i < len; i += 4)
	{
		samples[i + 0] = -samples[i + 0];
		samples[i + 1] = -samples[i + 1] >> 1;
		//samples[i + 2] = samples[i + 2];
		samples[i + 3] = samples[i + 3] >> 1;
	}

	fir_interleaved(cnv, samples, len);
	delay_interleaved(cnv, samples + 1, len);
}

void iqconverter_int16_process(iqconverter_int16_t *cnv, int16_t *samples, int len)
{
	remove_dc(cnv, samples, len);
	translate_fs_4(cnv, samples, len);
}
