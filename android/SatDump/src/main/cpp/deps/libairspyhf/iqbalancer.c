/*
Copyright (c) 2016-2018, Youssef Touil <youssef@airspy.com>
Copyright (c) 2018, Leif Asbrink <leif@sm5bsz.com>

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

		Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
		Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the
		documentation and/or other materials provided with the distribution.
		Neither the name of Airspy HF+ nor the names of its contributors may be used to endorse or promote products derived from this software
		without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "iqbalancer.h"

#ifndef MATH_PI
#define MATH_PI 3.14159265359
#endif

#define EPSILON 0.01f
#define WorkingBufferLength (FFTBins * (1 + FFTIntegration / FFTOverlap))

struct iq_balancer_t
{
	float phase;
	float last_phase;

	float amplitude;
	float last_amplitude;

	float iavg;
	float qavg;
	float integrated_total_power;
	float integrated_image_power;
	float maximum_image_power;

	float raw_phases[MaxLookback];
	float raw_amplitudes[MaxLookback];

	int skipped_buffers;
	int buffers_to_skip;
	int working_buffer_pos;
	int fft_integration;
	int fft_overlap;
	int correlation_integration;

	int no_of_avg;
	int no_of_raw;
	int raw_ptr;
	int optimal_bin;
	int reset_flag;
	int *power_flag;

	complex_t *corr;
	complex_t *corr_plus;
	complex_t *working_buffer;
	float *boost;
};

static uint8_t __lib_initialized = 0;
static float __fft_window[FFTBins];
static float __boost_window[FFTBins];

static void __init_library()
{
	int i;

	if (__lib_initialized)
	{
		return;
	}

	const int length = FFTBins - 1;

	for (i = 0; i <= length; i++)
	{
		__fft_window[i] = (float)(
			+0.35875f
			- 0.48829f * cos(2.0 * MATH_PI * i / length)
			+ 0.14128f * cos(4.0 * MATH_PI * i / length)
			- 0.01168f * cos(6.0 * MATH_PI * i / length)
			);
		__boost_window[i] = (float)(1.0 / BoostFactor + 1.0 / exp(pow(i * 2.0 / BinsToOptimize, 2.0)));
	}

	__lib_initialized = 1;
}

static void window(complex_t *buffer, int length)
{
	int i;
	for (i = 0; i < length; i++)
	{
		buffer[i].re *= __fft_window[i];
		buffer[i].im *= __fft_window[i];
	}
}

static void fft(complex_t *buffer, int length)
{
	int nm1 = length - 1;
	int nd2 = length / 2;
	int i, j, jm1, k, l, m, le, le2, ip;
	complex_t u, t, r;

	m = 0;
	i = length;
	while (i > 1)
	{
		++m;
		i = (i >> 1);
	}

	j = nd2;

	for (i = 1; i < nm1; ++i)
	{
		if (i < j)
		{
			t = buffer[j];
			buffer[j] = buffer[i];
			buffer[i] = t;
		}

		k = nd2;

		while (k <= j)
		{
			j = j - k;
			k = k / 2;
		}

		j += k;
	}

	for (l = 1; l <= m; ++l)
	{
		le = 1 << l;
		le2 = le / 2;

		u.re = 1.0f;
		u.im = 0.0f;

		r.re = (float)cos(MATH_PI / le2);
		r.im = (float)-sin(MATH_PI / le2);

		for (j = 1; j <= le2; ++j)
		{
			jm1 = j - 1;

			for (i = jm1; i <= nm1; i += le)
			{
				ip = i + le2;

				t.re = u.re * buffer[ip].re - u.im * buffer[ip].im;
				t.im = u.im * buffer[ip].re + u.re * buffer[ip].im;

				buffer[ip].re = buffer[i].re - t.re;
				buffer[ip].im = buffer[i].im - t.im;

				buffer[i].re += t.re;
				buffer[i].im += t.im;
			}

			t.re = u.re * r.re - u.im * r.im;
			t.im = u.im * r.re + u.re * r.im;

			u.re = t.re;
			u.im = t.im;
		}
	}

	for (i = 0; i < nd2; i++)
	{
		j = nd2 + i;
		t = buffer[i];
		buffer[i] = buffer[j];
		buffer[j] = t;
	}
}

static void cancel_dc(struct iq_balancer_t *iq_balancer, complex_t* iq, int length)
{
	int i;
	float iavg = iq_balancer->iavg;
	float qavg = iq_balancer->qavg;

	for (i = 0; i < length; i++)
	{
		iavg += DcTimeConst * (iq[i].re - iavg);
		qavg += DcTimeConst * (iq[i].im - qavg);

		iq[i].re -= iavg;
		iq[i].im -= qavg;
	}

	iq_balancer->iavg = iavg;
	iq_balancer->qavg = qavg;
}

static float adjust_benchmark(struct iq_balancer_t *iq_balancer, complex_t *iq, float phase, float amplitude, int skip_power_calculation)
{
	int i;
	float sum = 0;
	for (i = 0; i < FFTBins; i++)
	{
		float re = iq[i].re;
		float im = iq[i].im;

		iq[i].re += phase * im;
		iq[i].im += phase * re;

		iq[i].re *= 1 + amplitude;
		iq[i].im *= 1 - amplitude;
		if (!skip_power_calculation)
			sum += re * re + im * im;
	}
	return sum;
}

static complex_t multiply_complex_complex(complex_t *a, const complex_t *b)
{
	complex_t result;
	result.re = a->re * b->re - a->im * b->im;
	result.im = a->im * b->re + a->re * b->im;
	return result;
}

static int compute_corr(struct iq_balancer_t *iq_balancer, complex_t* iq, complex_t* ccorr, int length, int step)
{
	complex_t cc, fftPtr[FFTBins];
	int n, m;
	int i, j;
	int count = 0;
	float power;
	float phase = iq_balancer->phase + step * PhaseStep;
	float amplitude = iq_balancer->amplitude + step * AmplitudeStep;

	for (n = 0, m = 0; n <= length - FFTBins && m < iq_balancer->fft_integration; n += FFTBins / iq_balancer->fft_overlap, m++)
	{
		memcpy(fftPtr, iq + n, FFTBins * sizeof(complex_t));
		power = adjust_benchmark(iq_balancer, fftPtr, phase, amplitude, step);
		if (step == 0)
		{
			if (power > MinimumPower)
			{
				iq_balancer->power_flag[m] = 1;
				iq_balancer->integrated_total_power += power;
			}
			else
			{
				iq_balancer->power_flag[m] = 0;
			}
		}
		if (iq_balancer->power_flag[m] == 1)
		{
			count++;
			window(fftPtr, FFTBins);
			fft(fftPtr, FFTBins);
			for (i = EdgeBinsToSkip, j = FFTBins - EdgeBinsToSkip; i <= FFTBins - EdgeBinsToSkip; i++, j--)
			{
				cc = multiply_complex_complex(fftPtr + i, fftPtr + j);
				ccorr[i].re += cc.re;
				ccorr[i].im += cc.im;

				ccorr[j].re = ccorr[i].re;
				ccorr[j].im = ccorr[i].im;
			}
			if (step == 0)
			{
				for (i = EdgeBinsToSkip; i <= FFTBins - EdgeBinsToSkip; i++)
				{
					power = fftPtr[i].re * fftPtr[i].re + fftPtr[i].im * fftPtr[i].im;
					iq_balancer->boost[i] += power;
					if (iq_balancer->optimal_bin == FFTBins / 2)
					{
						iq_balancer->integrated_image_power += power;
					}
					else
					{
						iq_balancer->integrated_image_power += power * __boost_window[abs(FFTBins - i - iq_balancer->optimal_bin)];
					}
				}
			}
		}
	}

	return count;
}

static complex_t utility(struct iq_balancer_t *iq_balancer, complex_t* ccorr)
{
	int i;
	int j;
	float invskip = 1.0f / EdgeBinsToSkip;
	complex_t acc = { 0, 0 };
	for (i = EdgeBinsToSkip, j = FFTBins - EdgeBinsToSkip; i <= FFTBins - EdgeBinsToSkip; i++, j--)
	{
		int distance = abs(i - FFTBins / 2);
		if (distance > CenterBinsToSkip)
		{
			float weight = (distance > EdgeBinsToSkip) ? 1.0f : (distance * invskip);
			if (iq_balancer->optimal_bin != FFTBins / 2)
			{
				weight *= __boost_window[abs(iq_balancer->optimal_bin - i)];
			}
			weight *= iq_balancer->boost[j] / (iq_balancer->boost[i] + EPSILON);
			acc.re += ccorr[i].re * weight;
			acc.im += ccorr[i].im * weight;
		}
	}
	return acc;
}

static void estimate_imbalance(struct iq_balancer_t *iq_balancer, complex_t* iq, int length)
{
	int i, j;
	float amplitude, phase, mu;
	complex_t a, b;

	if (iq_balancer->reset_flag)
	{
		iq_balancer->reset_flag = 0;
		iq_balancer->no_of_avg = -BuffersToSkipOnReset;
		iq_balancer->maximum_image_power = 0;
	}

	if (iq_balancer->no_of_avg < 0)
	{
		iq_balancer->no_of_avg++;
		return;
	}
	else if (iq_balancer->no_of_avg == 0)
	{
		iq_balancer->integrated_image_power = 0;
		iq_balancer->integrated_total_power = 0;
		memset(iq_balancer->boost, 0, FFTBins * sizeof(float));
		memset(iq_balancer->corr, 0, FFTBins * sizeof(complex_t));
		memset(iq_balancer->corr_plus, 0, FFTBins * sizeof(complex_t));
	}

	iq_balancer->maximum_image_power *= MaxPowerDecay;

	i = compute_corr(iq_balancer, iq, iq_balancer->corr, length, 0);
	if (i == 0)
		return;

	iq_balancer->no_of_avg += i;
	compute_corr(iq_balancer, iq, iq_balancer->corr_plus, length, 1);

	if (iq_balancer->no_of_avg <= iq_balancer->correlation_integration * iq_balancer->fft_integration)
		return;

	iq_balancer->no_of_avg = 0;

	if (iq_balancer->optimal_bin == FFTBins / 2)
	{
		if (iq_balancer->integrated_total_power < iq_balancer->maximum_image_power)
			return;
		iq_balancer->maximum_image_power = iq_balancer->integrated_total_power;
	}
	else
	{
		if (iq_balancer->integrated_image_power - iq_balancer->integrated_total_power * BoostWindowNorm < iq_balancer->maximum_image_power * PowerThreshold)
			return;
		iq_balancer->maximum_image_power = iq_balancer->integrated_image_power - iq_balancer->integrated_total_power * BoostWindowNorm;
	}

	a = utility(iq_balancer, iq_balancer->corr);
	b = utility(iq_balancer, iq_balancer->corr_plus);

	mu = a.im - b.im;
	if (fabs(mu) > MinDeltaMu)
	{
		mu = a.im / mu;
		if (mu < -MaxMu)
			mu = -MaxMu;
		else if (mu > MaxMu)
			mu = MaxMu;
	}
	else
	{
		mu = 0;
	}

	phase = iq_balancer->phase + PhaseStep * mu;

	mu = a.re - b.re;
	if (fabs(mu) > MinDeltaMu)
	{
		mu = a.re / mu;
		if (mu < -MaxMu)
			mu = -MaxMu;
		else if (mu > MaxMu)
			mu = MaxMu;
	}
	else
	{
		mu = 0;
	}

	amplitude = iq_balancer->amplitude + AmplitudeStep * mu;

	if (iq_balancer->no_of_raw < MaxLookback)
		iq_balancer->no_of_raw++;
	iq_balancer->raw_amplitudes[iq_balancer->raw_ptr] = amplitude;
	iq_balancer->raw_phases[iq_balancer->raw_ptr] = phase;
	i = iq_balancer->raw_ptr;
	for (j = 0; j < iq_balancer->no_of_raw - 1; j++)
	{
		i = (i + MaxLookback - 1) & (MaxLookback - 1);
		phase += iq_balancer->raw_phases[i];
		amplitude += iq_balancer->raw_amplitudes[i];
	}
	phase /= iq_balancer->no_of_raw;
	amplitude /= iq_balancer->no_of_raw;
	iq_balancer->raw_ptr = (iq_balancer->raw_ptr + 1) & (MaxLookback - 1);

	iq_balancer->phase = phase;
	iq_balancer->amplitude = amplitude;
}

static void adjust_phase_amplitude(struct iq_balancer_t *iq_balancer, complex_t* iq, int length)
{
	int i;
	float scale = 1.0f / (length - 1);

	for (i = 0; i < length; i++)
	{
		float phase = (i * iq_balancer->last_phase + (length - 1 - i) * iq_balancer->phase) * scale;
		float amplitude = (i * iq_balancer->last_amplitude + (length - 1 - i) * iq_balancer->amplitude) * scale;

		float re = iq[i].re;
		float im = iq[i].im;

		iq[i].re += phase * im;
		iq[i].im += phase * re;

		iq[i].re *= 1 + amplitude;
		iq[i].im *= 1 - amplitude;
	}

	iq_balancer->last_phase = iq_balancer->phase;
	iq_balancer->last_amplitude = iq_balancer->amplitude;
}

void ADDCALL iq_balancer_process(struct iq_balancer_t *iq_balancer, complex_t* iq, int length)
{
	int count;

	cancel_dc(iq_balancer, iq, length);

	count = WorkingBufferLength - iq_balancer->working_buffer_pos;
	if (count >= length)
	{
		count = length;
	}
	memcpy(iq_balancer->working_buffer + iq_balancer->working_buffer_pos, iq, count * sizeof(complex_t));
	iq_balancer->working_buffer_pos += count;
	if (iq_balancer->working_buffer_pos >= WorkingBufferLength)
	{
		iq_balancer->working_buffer_pos = 0;

		if (++iq_balancer->skipped_buffers > iq_balancer->buffers_to_skip)
		{
			iq_balancer->skipped_buffers = 0;
			estimate_imbalance(iq_balancer, iq_balancer->working_buffer, WorkingBufferLength);
		}
	}

	adjust_phase_amplitude(iq_balancer, iq, length);
}

void ADDCALL iq_balancer_set_optimal_point(struct iq_balancer_t *iq_balancer, float w)
{
	if (w < -0.5f)
	{
		w = -0.5f;
	}
	else if (w > 0.5f)
	{
		w = 0.5f;
	}

	iq_balancer->optimal_bin = (int)floor(FFTBins * (0.5 + w));
	iq_balancer->reset_flag = 1;
}

void ADDCALL iq_balancer_configure(struct iq_balancer_t *iq_balancer, int buffers_to_skip, int fft_integration, int fft_overlap, int correlation_integration)
{
	iq_balancer->buffers_to_skip = buffers_to_skip;
	iq_balancer->fft_integration = fft_integration;
	iq_balancer->fft_overlap = fft_overlap;
	iq_balancer->correlation_integration = correlation_integration;

	free(iq_balancer->power_flag);
	iq_balancer->power_flag = (int *)malloc(iq_balancer->fft_integration * sizeof(int));
	memset(iq_balancer->power_flag, 0, iq_balancer->fft_integration * sizeof(int));

	iq_balancer->reset_flag = 1;
}

struct iq_balancer_t * ADDCALL iq_balancer_create(float initial_phase, float initial_amplitude)
{
	struct iq_balancer_t *instance = (struct iq_balancer_t *) malloc(sizeof(struct iq_balancer_t));
	memset(instance, 0, sizeof(struct iq_balancer_t));

	instance->phase = initial_phase;
	instance->amplitude = initial_amplitude;

	instance->optimal_bin = FFTBins / 2;

	instance->buffers_to_skip = BuffersToSkip;
	instance->fft_integration = FFTIntegration;
	instance->fft_overlap = FFTOverlap;
	instance->correlation_integration = CorrelationIntegration;

	instance->corr = (complex_t *)malloc(FFTBins * sizeof(complex_t));
	instance->corr_plus = (complex_t *)malloc(FFTBins * sizeof(complex_t));
	instance->working_buffer = (complex_t *)malloc(WorkingBufferLength * sizeof(complex_t));
	instance->boost = (float *)malloc(FFTBins * sizeof(float));
	instance->power_flag = (int *)malloc(instance->fft_integration * sizeof(int));

	__init_library();
	return instance;
}

void ADDCALL iq_balancer_destroy(struct iq_balancer_t *iq_balancer)
{
	free(iq_balancer->corr);
	free(iq_balancer->corr_plus);
	free(iq_balancer->working_buffer);
	free(iq_balancer->boost);
	free(iq_balancer->power_flag);
	free(iq_balancer);
}
