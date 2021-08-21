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

#ifndef IQCONVERTER_FLOAT_H
#define IQCONVERTER_FLOAT_H

#include <stdint.h>

#define IQCONVERTER_NZEROS 2
#define IQCONVERTER_NPOLES 2

typedef struct {
	float avg;
	float hbc;
	int len;
	int fir_index;
	int delay_index;
	float *fir_kernel;
	float *fir_queue;
	float *delay_line;
} iqconverter_float_t;

iqconverter_float_t *iqconverter_float_create(const float *hb_kernel, int len);
void iqconverter_float_free(iqconverter_float_t *cnv);
void iqconverter_float_reset(iqconverter_float_t *cnv);
void iqconverter_float_process(iqconverter_float_t *cnv, float *samples, int len);

#endif // IQCONVERTER_FLOAT_H
