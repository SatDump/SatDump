/*
Copyright (c) 2013-2018, Youssef Touil <youssef@airspy.com>
Copyright (c) 2013-2017, Ian Gilmour <ian@sdrsharp.com>
Copyright (c) 2013-2017, Benjamin Vernoux <bvernoux@airspy.com>
Copyright (c) 2013, Michael Ossmann <mike@ossmann.com>
Copyright (c) 2012, Jared Boone <jared@sharebrained.com>

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

#ifdef _WIN32
#define HAVE_STRUCT_TIMESPEC
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb.h>
#include <pthread.h>
#include <math.h>

#include "iqbalancer.h"
#include "airspyhf.h"
#include "airspyhf_commands.h"

#ifndef bool
typedef int bool;
#define true 1
#define false 0
#endif

#ifndef M_PI
#define M_PI (3.14159265359)
#endif

#define MAX(a, b) ((a) > (b) ? a : b)
#define MIN(a, b) ((a) < (b) ? a : b)

#define SAMPLES_TO_TRANSFER (1024 * 2)
#define SERIAL_NUMBER_UNUSED (0)
#define RAW_BUFFER_COUNT (8)
#define AIRSPYHF_SERIAL_SIZE (28)

#define MAX_SAMPLERATE_INDEX (100)
#define DEFAULT_SAMPLERATE (768000)

#define CALIBRATION_MAGIC (0xA5CA71B0)

#define DEFAULT_IF_SHIFT (5000)
#define MIN_ZERO_IF_LO (180)
#define MIN_LOW_IF_LO (84)

#define STR_PREFIX_SERIAL_AIRSPYHF_SIZE (12)
static const char str_prefix_serial_airspyhf[STR_PREFIX_SERIAL_AIRSPYHF_SIZE] =
	{'A', 'I', 'R', 'S', 'P', 'Y', 'H', 'F', ' ', 'S', 'N', ':'};

#define INITIAL_PHASE (0.00006f)
#define INITIAL_AMPLITUDE (-0.0045f)

#pragma pack(push, 1)

typedef struct
{
	int16_t im;
	int16_t re;
} airspyhf_complex_int16_t;

#pragma pack(pop)

typedef struct airspyhf_device
{
	libusb_context *usb_context;
	libusb_device_handle *usb_device;
	struct libusb_transfer **transfers;
	airspyhf_sample_block_cb_fn callback;
	pthread_t transfer_thread;
	pthread_t consumer_thread;
	pthread_cond_t consumer_cv;
	pthread_mutex_t consumer_mp;
	uint32_t supported_samplerate_count;
	uint32_t *supported_samplerates;
	uint8_t *samplerate_architectures;
	volatile uint32_t current_samplerate;
	volatile uint32_t freq_hz;
	volatile uint32_t freq_khz;
	volatile int32_t freq_shift;
	volatile int32_t calibration_ppb;
	volatile float optimal_point;
	uint8_t enable_dsp;
	uint8_t is_low_if;
	float filter_gain;
	airspyhf_complex_float_t vec;
	struct iq_balancer_t *iq_balancer;
	uint32_t transfer_count;
	uint32_t buffer_size;
	uint32_t dropped_buffers;
	uint32_t dropped_buffers_queue[RAW_BUFFER_COUNT];
	airspyhf_complex_int16_t *received_samples_queue[RAW_BUFFER_COUNT];
	volatile bool streaming;
	volatile bool stop_requested;
	volatile int received_samples_queue_head;
	volatile int received_samples_queue_tail;
	volatile int received_buffer_count;
	airspyhf_complex_float_t *output_buffer;
	void *ctx;
} airspyhf_device_t;

typedef struct calibration_record
{
	uint32_t magic_number;
	int32_t calibration_ppb;
} calibration_record_t;

static const uint16_t airspyhf_usb_vid = 0x03EB;
static const uint16_t airspyhf_usb_pid = 0x800C;

static int airspyhf_config_read(airspyhf_device_t *device, uint8_t *buffer, uint16_t length);

static int cancel_transfers(airspyhf_device_t *device)
{
	uint32_t transfer_index;

	if (device->transfers != NULL)
	{
		for (transfer_index = 0; transfer_index < device->transfer_count; transfer_index++)
		{
			if (device->transfers[transfer_index] != NULL)
			{
				libusb_cancel_transfer(device->transfers[transfer_index]);
			}
		}
		return AIRSPYHF_SUCCESS;
	}
	else
	{
		return AIRSPYHF_ERROR;
	}
}

static int free_transfers(airspyhf_device_t *device)
{
	int i;
	uint32_t transfer_index;

	if (device->transfers != NULL)
	{
		free(device->output_buffer);
		device->output_buffer = NULL;

		for (transfer_index = 0; transfer_index < device->transfer_count; transfer_index++)
		{
			if (device->transfers[transfer_index] != NULL)
			{
				free(device->transfers[transfer_index]->buffer);
				libusb_free_transfer(device->transfers[transfer_index]);
				device->transfers[transfer_index] = NULL;
			}
		}
		free(device->transfers);
		device->transfers = NULL;

		for (i = 0; i < RAW_BUFFER_COUNT; i++)
		{
			if (device->received_samples_queue[i] != NULL)
			{
				free(device->received_samples_queue[i]);
				device->received_samples_queue[i] = NULL;
			}
		}
	}

	return AIRSPYHF_SUCCESS;
}

static int allocate_transfers(airspyhf_device_t *const device)
{
	int i;
	uint32_t transfer_index;

	if (device->transfers == NULL)
	{
		device->output_buffer = (airspyhf_complex_float_t *)malloc((device->buffer_size / sizeof(airspyhf_complex_int16_t)) * sizeof(airspyhf_complex_float_t));

		for (i = 0; i < RAW_BUFFER_COUNT; i++)
		{
			device->received_samples_queue[i] = (airspyhf_complex_int16_t *)malloc(device->buffer_size);
			if (device->received_samples_queue[i] == NULL)
			{
				return AIRSPYHF_ERROR;
			}

			memset(device->received_samples_queue[i], 0, device->buffer_size);
		}

		device->transfers = (struct libusb_transfer **)calloc(device->transfer_count, sizeof(struct libusb_transfer));
		if (device->transfers == NULL)
		{
			return AIRSPYHF_ERROR;
		}

		for (transfer_index = 0; transfer_index < device->transfer_count; transfer_index++)
		{
			device->transfers[transfer_index] = libusb_alloc_transfer(0);
			if (device->transfers[transfer_index] == NULL)
			{
				return AIRSPYHF_ERROR;
			}

			libusb_fill_bulk_transfer(
				device->transfers[transfer_index],
				device->usb_device,
				0,
				(unsigned char *)malloc(device->buffer_size),
				device->buffer_size,
				NULL,
				device,
				0);

			if (device->transfers[transfer_index]->buffer == NULL)
			{
				return AIRSPYHF_ERROR;
			}
		}
		return AIRSPYHF_SUCCESS;
	}
	else
	{
		return AIRSPYHF_ERROR;
	}
}

static int prepare_transfers(airspyhf_device_t *device, const uint_fast8_t endpoint_address, libusb_transfer_cb_fn callback)
{
	int error;
	uint32_t transfer_index;

	if (device->transfers != NULL)
	{
		for (transfer_index = 0; transfer_index < device->transfer_count; transfer_index++)
		{
			device->transfers[transfer_index]->endpoint = endpoint_address;
			device->transfers[transfer_index]->callback = callback;

			error = libusb_submit_transfer(device->transfers[transfer_index]);
			if (error != 0)
			{
				return AIRSPYHF_ERROR;
			}
		}
		return AIRSPYHF_SUCCESS;
	}

	return AIRSPYHF_ERROR;
}

static void multiply_complex_complex(airspyhf_complex_float_t *a, const airspyhf_complex_float_t *b)
{
	float re = a->re * b->re - a->im * b->im;
	a->im = a->im * b->re + a->re * b->im;
	a->re = re;
}

static void multiply_complex_real(airspyhf_complex_float_t *a, const float b)
{
	a->re *= b;
	a->im *= b;
}

static inline void rotate_complex(airspyhf_complex_float_t *vec, const airspyhf_complex_float_t *rot)
{
	multiply_complex_complex(vec, rot);

	float norm = 1.99f - (vec->re * vec->re + vec->im * vec->im);

	multiply_complex_real(vec, norm);
}

static void convert_samples(airspyhf_device_t *device, airspyhf_complex_int16_t *src, airspyhf_complex_float_t *dest, int count)
{
	const float scale = 1.0f / 32768;

	int i;
	airspyhf_complex_float_t vec;
	airspyhf_complex_float_t rot;
	double angle;
	float conversion_gain;

	conversion_gain = scale * device->filter_gain;

	for (i = 0; i < count; i++)
	{
		dest[i].re = src[i].re * conversion_gain;
		dest[i].im = src[i].im * conversion_gain;
	}

	if (device->enable_dsp)
	{
		if (!device->is_low_if)
		{
			// Zero IF requires external IQ correction
			iq_balancer_process(device->iq_balancer, dest, count);
		}

		// Fine tuning
		if (device->freq_shift != 0)
		{
			angle = 2.0 * M_PI * device->freq_shift / (double)device->current_samplerate;

			vec = device->vec;

			rot.re = (float)cos(angle);
			rot.im = (float)-sin(angle);

			for (i = 0; i < count; i++)
			{
				rotate_complex(&vec, &rot);
				multiply_complex_complex(&dest[i], &vec);
			}

			device->vec = vec;
		}
	}
}

static void *consumer_threadproc(void *arg)
{
	int sample_count;
	airspyhf_complex_int16_t *input_samples;
	uint32_t dropped_buffers;
	airspyhf_device_t *device = (airspyhf_device_t *)arg;
	airspyhf_transfer_t transfer;

#ifdef _WIN32

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

#endif

	pthread_mutex_lock(&device->consumer_mp);

	while (device->streaming && !device->stop_requested)
	{
		while (device->received_buffer_count == 0 && device->streaming && !device->stop_requested)
		{
			pthread_cond_wait(&device->consumer_cv, &device->consumer_mp);
		}
		if (!device->streaming || device->stop_requested)
		{
			break;
		}

		input_samples = (airspyhf_complex_int16_t *)device->received_samples_queue[device->received_samples_queue_tail];
		dropped_buffers = device->dropped_buffers_queue[device->received_samples_queue_tail];
		device->received_samples_queue_tail = (device->received_samples_queue_tail + 1) & (RAW_BUFFER_COUNT - 1);

		pthread_mutex_unlock(&device->consumer_mp);

		sample_count = device->buffer_size / sizeof(airspyhf_complex_int16_t);

		convert_samples(device, input_samples, device->output_buffer, sample_count);

		transfer.device = device;
		transfer.ctx = device->ctx;
		transfer.samples = device->output_buffer;
		transfer.sample_count = sample_count;
		transfer.dropped_samples = (uint64_t)dropped_buffers * (uint64_t)sample_count;

		if (device->callback(&transfer) != 0)
		{
			device->stop_requested = true;
		}

		pthread_mutex_lock(&device->consumer_mp);
		device->received_buffer_count--;
	}

	pthread_mutex_unlock(&device->consumer_mp);

	pthread_exit(NULL);

	return NULL;
}

static void airspyhf_libusb_transfer_callback(struct libusb_transfer *usb_transfer)
{
	airspyhf_complex_int16_t *temp;
	airspyhf_device_t *device = (airspyhf_device_t *)usb_transfer->user_data;

	if (!device->streaming || device->stop_requested)
	{
		return;
	}

	if (usb_transfer->status == LIBUSB_TRANSFER_COMPLETED && usb_transfer->actual_length == usb_transfer->length)
	{
		pthread_mutex_lock(&device->consumer_mp);

		if (device->received_buffer_count < RAW_BUFFER_COUNT)
		{
			temp = device->received_samples_queue[device->received_samples_queue_head];
			device->received_samples_queue[device->received_samples_queue_head] = (airspyhf_complex_int16_t *)usb_transfer->buffer;
			usb_transfer->buffer = (uint8_t *)temp;

			device->dropped_buffers_queue[device->received_samples_queue_head] = device->dropped_buffers;
			device->dropped_buffers = 0;

			device->received_samples_queue_head = (device->received_samples_queue_head + 1) & (RAW_BUFFER_COUNT - 1);
			device->received_buffer_count++;

			pthread_cond_signal(&device->consumer_cv);
		}
		else
		{
			device->dropped_buffers++;
		}

		pthread_mutex_unlock(&device->consumer_mp);

		if (libusb_submit_transfer(usb_transfer) != 0)
		{
			device->streaming = false;
		}
	}
	else
	{
		device->streaming = false;
	}
}

static void *transfer_threadproc(void *arg)
{
	airspyhf_device_t *device = (struct airspyhf_device *)arg;
	int error;
	struct timeval timeout = {0, 500000};

#ifdef _WIN32

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

#endif

	while (device->streaming && !device->stop_requested)
	{
		error = libusb_handle_events_timeout_completed(device->usb_context, &timeout, NULL);
		if (error < 0)
		{
			if (error != LIBUSB_ERROR_INTERRUPTED)
				device->streaming = false;
		}
	}

	pthread_exit(NULL);

	return NULL;
}

static int kill_io_threads(airspyhf_device_t *device)
{
	if (device->streaming)
	{
		device->stop_requested = true;
		cancel_transfers(device);

		pthread_mutex_lock(&device->consumer_mp);
		pthread_cond_signal(&device->consumer_cv);
		pthread_mutex_unlock(&device->consumer_mp);

		pthread_join(device->transfer_thread, NULL);
		pthread_join(device->consumer_thread, NULL);

		device->stop_requested = false;
		device->streaming = false;
	}

	return AIRSPYHF_SUCCESS;
}

static int create_io_threads(airspyhf_device_t *device, airspyhf_sample_block_cb_fn callback)
{
	int result;
	pthread_attr_t attr;

	if (!device->streaming && !device->stop_requested)
	{
		device->callback = callback;
		device->streaming = true;

		result = prepare_transfers(device, LIBUSB_ENDPOINT_IN | AIRSPYHF_ENDPOINT_IN, (libusb_transfer_cb_fn)airspyhf_libusb_transfer_callback);
		if (result != AIRSPYHF_SUCCESS)
		{
			return result;
		}

		device->received_samples_queue_head = 0;
		device->received_samples_queue_tail = 0;
		device->received_buffer_count = 0;

		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

		result = pthread_create(&device->consumer_thread, &attr, consumer_threadproc, device);
		if (result != 0)
		{
			return AIRSPYHF_ERROR;
		}

		result = pthread_create(&device->transfer_thread, &attr, transfer_threadproc, device);
		if (result != 0)
		{
			return AIRSPYHF_ERROR;
		}

		pthread_attr_destroy(&attr);
	}
	else
	{
		return AIRSPYHF_ERROR;
	}

	return AIRSPYHF_SUCCESS;
}

static void airspyhf_open_exit(airspyhf_device_t *device)
{
	if (device->usb_device != NULL)
	{
		libusb_release_interface(device->usb_device, 0);
		libusb_close(device->usb_device);
		device->usb_device = NULL;
	}
	libusb_exit(device->usb_context);
	device->usb_context = NULL;
}

static int airspyhf_read_samplerates_from_fw(airspyhf_device_t *device, uint32_t *buffer, const uint32_t len)
{
	int result;

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		AIRSPYHF_GET_SAMPLERATES,
		0,
		len,
		(unsigned char *)buffer,
		(len > 0 ? len : 1) * (int16_t)sizeof(uint32_t),
		0);

	if (result < 1)
	{
		return AIRSPYHF_ERROR;
	}
	return AIRSPYHF_SUCCESS;
}

static int airspyhf_read_samplerate_architectures_from_fw(airspyhf_device_t *device, uint8_t *buffer, const uint32_t len)
{
	int result;

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		AIRSPYHF_GET_SAMPLERATE_ARCHITECTURES,
		0,
		len,
		(unsigned char *)buffer,
		(len > 0 ? len : 1) * (int16_t)sizeof(uint32_t),
		0);

	if (result < 1)
	{
		return AIRSPYHF_ERROR;
	}

	return AIRSPYHF_SUCCESS;
}

static void airspyhf_open_device(airspyhf_device_t *device,
								 int *ret,
								 uint16_t vid,
								 uint16_t pid,
								 uint64_t serial_number_val)
{
	int i;
	int result;
	libusb_device_handle **libusb_dev_handle;
	int serial_number_len;
	libusb_device_handle *dev_handle;
	libusb_device *dev;
	libusb_device **devices = NULL;

	ssize_t cnt;
	int serial_descriptor_index;
	struct libusb_device_descriptor device_descriptor;
	unsigned char serial_number[AIRSPYHF_SERIAL_SIZE + 1];

	libusb_dev_handle = &device->usb_device;
	*libusb_dev_handle = NULL;

	cnt = libusb_get_device_list(device->usb_context, &devices);
	if (cnt < 0)
	{
		*ret = AIRSPYHF_ERROR;
		return;
	}

	i = 0;
	while ((dev = devices[i++]) != NULL)
	{
		libusb_get_device_descriptor(dev, &device_descriptor);

		if ((device_descriptor.idVendor == vid) &&
			(device_descriptor.idProduct == pid))
		{
			if (serial_number_val != SERIAL_NUMBER_UNUSED)
			{
				serial_descriptor_index = device_descriptor.iSerialNumber;
				if (serial_descriptor_index > 0)
				{
					if (libusb_open(dev, libusb_dev_handle) != 0)
					{
						*libusb_dev_handle = NULL;
						continue;
					}
					dev_handle = *libusb_dev_handle;
					serial_number_len = libusb_get_string_descriptor_ascii(dev_handle,
																		   serial_descriptor_index,
																		   serial_number,
																		   sizeof(serial_number));

					if (serial_number_len == AIRSPYHF_SERIAL_SIZE && !memcmp(str_prefix_serial_airspyhf, serial_number, STR_PREFIX_SERIAL_AIRSPYHF_SIZE))
					{
						uint64_t serial = SERIAL_NUMBER_UNUSED;
						// use same code to detemine device's serial number as in airspyhf_list_devices()
						{
							char *start, *end;
							serial_number[AIRSPYHF_SERIAL_SIZE] = 0;
							start = (char *)(serial_number + STR_PREFIX_SERIAL_AIRSPYHF_SIZE);
							end = NULL;
							serial = strtoull(start, &end, 16);
						}
						if (serial == serial_number_val)
						{
							result = libusb_set_configuration(dev_handle, 1);
							if (result != 0)
							{
								libusb_close(dev_handle);
								*libusb_dev_handle = NULL;
								continue;
							}
							result = libusb_claim_interface(dev_handle, 0);
							if (result != 0)
							{
								libusb_close(dev_handle);
								*libusb_dev_handle = NULL;
								continue;
							}

							result = libusb_set_interface_alt_setting(dev_handle, 0, 1);
							if (result != 0)
							{
								libusb_close(dev_handle);
								*libusb_dev_handle = NULL;
								continue;
							}

							break;
						}
						else
						{
							libusb_close(dev_handle);
							*libusb_dev_handle = NULL;
							continue;
						}
					}
					else
					{
						libusb_close(dev_handle);
						*libusb_dev_handle = NULL;
						continue;
					}
				}
			}
			else
			{
				if (libusb_open(dev, libusb_dev_handle) == 0)
				{
					dev_handle = *libusb_dev_handle;
					result = libusb_set_configuration(dev_handle, 1);
					if (result != 0)
					{
						libusb_close(dev_handle);
						*libusb_dev_handle = NULL;
						continue;
					}
					result = libusb_claim_interface(dev_handle, 0);
					if (result != 0)
					{
						libusb_close(dev_handle);
						*libusb_dev_handle = NULL;
						continue;
					}

					result = libusb_set_interface_alt_setting(dev_handle, 0, 1);
					if (result != 0)
					{
						libusb_close(dev_handle);
						*libusb_dev_handle = NULL;
						continue;
					}
					break;
				}
			}
		}
	}
	libusb_free_device_list(devices, 1);

	dev_handle = device->usb_device;
	if (dev_handle == NULL)
	{
		*ret = AIRSPYHF_ERROR;
		return;
	}

	*ret = AIRSPYHF_SUCCESS;
	return;
}

static void airspyhf_open2_device(airspyhf_device_t *device,
								  int *ret,
								  int fd,
								  const char *devicePath)
{
	int i;
	int result;
	libusb_device_handle **libusb_dev_handle;
	int serial_number_len;
	libusb_device_handle *dev_handle;
	libusb_device *dev;

	ssize_t cnt;
	int serial_descriptor_index;
	struct libusb_device_descriptor device_descriptor;
	unsigned char serial_number[AIRSPYHF_SERIAL_SIZE + 1];

	libusb_dev_handle = &device->usb_device;
	*libusb_dev_handle = NULL;

	dev = libusb_get_device2(device->usb_context, devicePath);

	if (!dev)
	{
		*ret = AIRSPYHF_ERROR;
		return;
	}

	if (libusb_open2(dev, libusb_dev_handle, fd) == 0)
	{
		dev_handle = *libusb_dev_handle;
		result = libusb_set_configuration(dev_handle, 1);
		if (result != 0)
		{
			libusb_close(dev_handle);
			*libusb_dev_handle = NULL;
		}
		result = libusb_claim_interface(dev_handle, 0);
		if (result != 0)
		{
			libusb_close(dev_handle);
			*libusb_dev_handle = NULL;
		}

		result = libusb_set_interface_alt_setting(dev_handle, 0, 1);
		if (result != 0)
		{
			libusb_close(dev_handle);
			*libusb_dev_handle = NULL;
		}
	}

	dev_handle = device->usb_device;
	if (dev_handle == NULL)
	{
		*ret = AIRSPYHF_ERROR;
		return;
	}

	*ret = AIRSPYHF_SUCCESS;
	return;
}

int airspyhf_list_devices(uint64_t *serials, int count)
{
	libusb_device_handle *libusb_dev_handle;
	struct libusb_context *context;
	libusb_device **devices = NULL;
	libusb_device *dev;
	struct libusb_device_descriptor device_descriptor;

	int serial_descriptor_index;
	int serial_number_len;
	int output_count;
	int i;
	unsigned char serial_number[AIRSPYHF_SERIAL_SIZE + 1];

	if (serials)
	{
		memset(serials, 0, sizeof(uint64_t) * count);
	}

	if (libusb_init(&context) != 0)
	{
		return AIRSPYHF_ERROR;
	}

	if (libusb_get_device_list(context, &devices) < 0)
	{
		return AIRSPYHF_ERROR;
	}

	i = 0;
	output_count = 0;
	while ((dev = devices[i++]) != NULL && (!serials || output_count < count))
	{
		libusb_get_device_descriptor(dev, &device_descriptor);

		if ((device_descriptor.idVendor == airspyhf_usb_vid) &&
			(device_descriptor.idProduct == airspyhf_usb_pid))
		{
			serial_descriptor_index = device_descriptor.iSerialNumber;
			if (serial_descriptor_index > 0)
			{
				if (libusb_open(dev, &libusb_dev_handle) != 0)
				{
					continue;
				}

				serial_number_len = libusb_get_string_descriptor_ascii(libusb_dev_handle,
																	   serial_descriptor_index,
																	   serial_number,
																	   sizeof(serial_number));

				if (serial_number_len == AIRSPYHF_SERIAL_SIZE && !memcmp(str_prefix_serial_airspyhf, serial_number, STR_PREFIX_SERIAL_AIRSPYHF_SIZE))
				{
					char *start, *end;
					uint64_t serial;

					serial_number[AIRSPYHF_SERIAL_SIZE] = 0;
					start = (char *)(serial_number + STR_PREFIX_SERIAL_AIRSPYHF_SIZE);
					end = NULL;
					serial = strtoull(start, &end, 16);
					if (serial == 0 && start == end)
					{
						libusb_close(libusb_dev_handle);
						continue;
					}

					if (serials)
					{
						serials[output_count] = serial;
					}
					output_count++;
				}

				libusb_close(libusb_dev_handle);
			}
		}
	}

	libusb_free_device_list(devices, 1);
	libusb_exit(context);
	return output_count;
}

static int airspyhf_open_init(airspyhf_device_t **device, uint64_t serial_number)
{
	airspyhf_device_t *lib_device;
	calibration_record_t record;
	int libusb_error;
	int result;

	*device = NULL;

	lib_device = (airspyhf_device_t *)calloc(1, sizeof(airspyhf_device_t));
	if (lib_device == NULL)
	{
		return AIRSPYHF_ERROR;
	}

	libusb_error = libusb_init(&lib_device->usb_context);
	if (libusb_error != 0)
	{
		free(lib_device);
		return AIRSPYHF_ERROR;
	}

	airspyhf_open_device(lib_device,
						 &result,
						 airspyhf_usb_vid,
						 airspyhf_usb_pid,
						 serial_number);

	if (lib_device->usb_device == NULL)
	{
		libusb_exit(lib_device->usb_context);
		free(lib_device);
		return result;
	}

	lib_device->transfers = NULL;
	lib_device->callback = NULL;
	lib_device->transfer_count = 16;
	lib_device->buffer_size = SAMPLES_TO_TRANSFER * sizeof(airspyhf_complex_int16_t);
	lib_device->streaming = false;
	lib_device->stop_requested = false;

	lib_device->supported_samplerates = NULL;
	lib_device->samplerate_architectures = NULL;

	result = airspyhf_read_samplerates_from_fw(lib_device, &lib_device->supported_samplerate_count, 0);
	if (result == AIRSPYHF_SUCCESS)
	{
		lib_device->supported_samplerates = (uint32_t *)malloc(lib_device->supported_samplerate_count * sizeof(uint32_t));
		result = airspyhf_read_samplerates_from_fw(lib_device, lib_device->supported_samplerates, lib_device->supported_samplerate_count);
		if (result == AIRSPYHF_SUCCESS)
		{
			lib_device->samplerate_architectures = (uint8_t *)malloc(lib_device->supported_samplerate_count * sizeof(uint8_t));
			result = airspyhf_read_samplerate_architectures_from_fw(lib_device, lib_device->samplerate_architectures, lib_device->supported_samplerate_count);
			if (result != AIRSPYHF_SUCCESS)
			{
				memset(lib_device->samplerate_architectures, 0, lib_device->supported_samplerate_count * sizeof(uint8_t)); // Asume Zero IF for all
				result = AIRSPYHF_SUCCESS;																				   // Clear this error for backward compatibility.
			}
		}
		else
		{
			free(lib_device->supported_samplerates);
			lib_device->supported_samplerates = NULL;
		}
	}

	if (result != AIRSPYHF_SUCCESS)
	{
		// Assume one default sample rate with Zero IF

		lib_device->supported_samplerate_count = 1;
		lib_device->supported_samplerates = (uint32_t *)malloc(lib_device->supported_samplerate_count * sizeof(uint32_t));
		lib_device->supported_samplerates[0] = DEFAULT_SAMPLERATE;

		lib_device->samplerate_architectures = (uint8_t *)malloc(lib_device->supported_samplerate_count * sizeof(uint8_t));
		lib_device->samplerate_architectures[0] = 0;
	}

	lib_device->current_samplerate = lib_device->supported_samplerates[0];
	lib_device->is_low_if = lib_device->samplerate_architectures[0];

	result = allocate_transfers(lib_device);
	if (result != 0)
	{
		airspyhf_open_exit(lib_device);
		free(lib_device);
		return AIRSPYHF_ERROR;
	}

	pthread_cond_init(&lib_device->consumer_cv, NULL);
	pthread_mutex_init(&lib_device->consumer_mp, NULL);

	lib_device->freq_hz = 0;
	lib_device->freq_khz = 0;
	lib_device->freq_shift = 0;
	lib_device->vec.re = 1.0f;
	lib_device->vec.im = 0.0f;
	lib_device->optimal_point = 0.0f;
	lib_device->filter_gain = 1.0f;
	lib_device->enable_dsp = 1;

	if (airspyhf_config_read(lib_device, (uint8_t *)&record, sizeof(record)) == AIRSPYHF_SUCCESS)
	{
		if (record.magic_number == CALIBRATION_MAGIC)
		{
			lib_device->calibration_ppb = record.calibration_ppb;
		}
		else
		{
			lib_device->calibration_ppb = 0;
		}
	}
	else
	{
		lib_device->calibration_ppb = 0;
	}

	lib_device->iq_balancer = iq_balancer_create(INITIAL_PHASE, INITIAL_AMPLITUDE);

	*device = lib_device;

	return AIRSPYHF_SUCCESS;
}

static int airspyhf_open2_init(airspyhf_device_t **device, uint64_t serial_number, int fd, const char *devicePath)
{
	airspyhf_device_t *lib_device;
	calibration_record_t record;
	int libusb_error;
	int result;

	*device = NULL;

	lib_device = (airspyhf_device_t *)calloc(1, sizeof(airspyhf_device_t));
	if (lib_device == NULL)
	{
		return AIRSPYHF_ERROR;
	}

	libusb_error = libusb_init(&lib_device->usb_context);
	if (libusb_error != 0)
	{
		free(lib_device);
		return AIRSPYHF_ERROR;
	}

	airspyhf_open2_device(lib_device,
						  &result,
						  fd, devicePath);

	if (lib_device->usb_device == NULL)
	{
		libusb_exit(lib_device->usb_context);
		free(lib_device);
		return result;
	}

	lib_device->transfers = NULL;
	lib_device->callback = NULL;
	lib_device->transfer_count = 16;
	lib_device->buffer_size = SAMPLES_TO_TRANSFER * sizeof(airspyhf_complex_int16_t);
	lib_device->streaming = false;
	lib_device->stop_requested = false;

	lib_device->supported_samplerates = NULL;
	lib_device->samplerate_architectures = NULL;

	result = airspyhf_read_samplerates_from_fw(lib_device, &lib_device->supported_samplerate_count, 0);
	if (result == AIRSPYHF_SUCCESS)
	{
		lib_device->supported_samplerates = (uint32_t *)malloc(lib_device->supported_samplerate_count * sizeof(uint32_t));
		result = airspyhf_read_samplerates_from_fw(lib_device, lib_device->supported_samplerates, lib_device->supported_samplerate_count);
		if (result == AIRSPYHF_SUCCESS)
		{
			lib_device->samplerate_architectures = (uint8_t *)malloc(lib_device->supported_samplerate_count * sizeof(uint8_t));
			result = airspyhf_read_samplerate_architectures_from_fw(lib_device, lib_device->samplerate_architectures, lib_device->supported_samplerate_count);
			if (result != AIRSPYHF_SUCCESS)
			{
				memset(lib_device->samplerate_architectures, 0, lib_device->supported_samplerate_count * sizeof(uint8_t)); // Asume Zero IF for all
				result = AIRSPYHF_SUCCESS;																				   // Clear this error for backward compatibility.
			}
		}
		else
		{
			free(lib_device->supported_samplerates);
			lib_device->supported_samplerates = NULL;
		}
	}

	if (result != AIRSPYHF_SUCCESS)
	{
		// Assume one default sample rate with Zero IF

		lib_device->supported_samplerate_count = 1;
		lib_device->supported_samplerates = (uint32_t *)malloc(lib_device->supported_samplerate_count * sizeof(uint32_t));
		lib_device->supported_samplerates[0] = DEFAULT_SAMPLERATE;

		lib_device->samplerate_architectures = (uint8_t *)malloc(lib_device->supported_samplerate_count * sizeof(uint8_t));
		lib_device->samplerate_architectures[0] = 0;
	}

	lib_device->current_samplerate = lib_device->supported_samplerates[0];
	lib_device->is_low_if = lib_device->samplerate_architectures[0];

	result = allocate_transfers(lib_device);
	if (result != 0)
	{
		airspyhf_open_exit(lib_device);
		free(lib_device);
		return AIRSPYHF_ERROR;
	}

	pthread_cond_init(&lib_device->consumer_cv, NULL);
	pthread_mutex_init(&lib_device->consumer_mp, NULL);

	lib_device->freq_hz = 0;
	lib_device->freq_khz = 0;
	lib_device->freq_shift = 0;
	lib_device->vec.re = 1.0f;
	lib_device->vec.im = 0.0f;
	lib_device->optimal_point = 0.0f;
	lib_device->filter_gain = 1.0f;
	lib_device->enable_dsp = 1;

	if (airspyhf_config_read(lib_device, (uint8_t *)&record, sizeof(record)) == AIRSPYHF_SUCCESS)
	{
		if (record.magic_number == CALIBRATION_MAGIC)
		{
			lib_device->calibration_ppb = record.calibration_ppb;
		}
		else
		{
			lib_device->calibration_ppb = 0;
		}
	}
	else
	{
		lib_device->calibration_ppb = 0;
	}

	lib_device->iq_balancer = iq_balancer_create(INITIAL_PHASE, INITIAL_AMPLITUDE);

	*device = lib_device;

	return AIRSPYHF_SUCCESS;
}

void ADDCALL airspyhf_lib_version(airspyhf_lib_version_t *lib_version)
{
	lib_version->major_version = AIRSPYHF_VER_MAJOR;
	lib_version->minor_version = AIRSPYHF_VER_MINOR;
	lib_version->revision = AIRSPYHF_VER_REVISION;
}

int ADDCALL airspyhf_open_sn(airspyhf_device_t **device, uint64_t serial_number)
{
	int result;

	result = airspyhf_open_init(device, serial_number);
	return result;
}

int ADDCALL airspyhf_open(airspyhf_device_t **device)
{
	int result;

	result = airspyhf_open_init(device, SERIAL_NUMBER_UNUSED);
	return result;
}

int ADDCALL airspyhf_open2(airspyhf_device_t **device, int fd, const char *devicePath)
{
	int result;

	result = airspyhf_open2_init(device, SERIAL_NUMBER_UNUSED, fd, devicePath);
	return result;
}

int ADDCALL airspyhf_close(airspyhf_device_t *device)
{
	int result;

	result = AIRSPYHF_SUCCESS;

	if (device != NULL)
	{
		result = airspyhf_stop(device);

		pthread_cond_destroy(&device->consumer_cv);
		pthread_mutex_destroy(&device->consumer_mp);

		airspyhf_open_exit(device);
		free_transfers(device);
		free(device->supported_samplerates);
		free(device->samplerate_architectures);
		iq_balancer_destroy(device->iq_balancer);
		free(device);
	}

	return result;
}

int ADDCALL airspyhf_get_output_size(airspyhf_device_t *device)
{
	// Todo: Make this configurable
	return SAMPLES_TO_TRANSFER;
}

int ADDCALL airspyhf_is_low_if(airspyhf_device_t *device)
{
	return device->is_low_if;
}

int ADDCALL airspyhf_get_samplerates(airspyhf_device_t *device, uint32_t *buffer, const uint32_t len)
{
	if (len == 0)
	{
		*buffer = device->supported_samplerate_count;
	}
	else if (len <= device->supported_samplerate_count)
	{
		memcpy(buffer, device->supported_samplerates, len * sizeof(uint32_t));
	}
	else
	{
		return AIRSPYHF_ERROR;
	}

	return AIRSPYHF_SUCCESS;
}

int ADDCALL airspyhf_set_samplerate(airspyhf_device_t *device, uint32_t samplerate)
{
	int result;
	uint32_t i;
	uint8_t gain;

	if (samplerate > MAX_SAMPLERATE_INDEX)
	{
		for (i = 0; i < device->supported_samplerate_count; i++)
		{
			if (samplerate == device->supported_samplerates[i])
			{
				samplerate = i;
				break;
			}
		}
	}

	if (samplerate >= device->supported_samplerate_count)
	{
		return AIRSPYHF_ERROR;
	}

	device->current_samplerate = device->supported_samplerates[samplerate];
	device->is_low_if = device->samplerate_architectures[samplerate];

	libusb_clear_halt(device->usb_device, LIBUSB_ENDPOINT_IN | 1);

	if (!device->is_low_if && device->freq_khz < MIN_ZERO_IF_LO)
	{
		uint8_t buf[4];
		buf[0] = (uint8_t)((MIN_ZERO_IF_LO >> 24) & 0xff);
		buf[1] = (uint8_t)((MIN_ZERO_IF_LO >> 16) & 0xff);
		buf[2] = (uint8_t)((MIN_ZERO_IF_LO >> 8) & 0xff);
		buf[3] = (uint8_t)((MIN_ZERO_IF_LO)&0xff);

		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPYHF_SET_FREQ,
			0,
			0,
			(unsigned char *)&buf,
			sizeof(buf),
			0);

		if (result < sizeof(buf))
		{
			return AIRSPYHF_ERROR;
		}

		device->freq_khz = MIN_ZERO_IF_LO;
	}

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		AIRSPYHF_SET_SAMPLERATE,
		0,
		samplerate,
		NULL,
		0,
		0);

	if (result != 0)
	{
		return AIRSPYHF_ERROR;
	}

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		AIRSPYHF_GET_FILTER_GAIN,
		0,
		0,
		&gain,
		sizeof(uint8_t),
		0);

	if (result == sizeof(uint8_t))
	{
		device->filter_gain = powf(10.0f, (float)gain * -0.05f);
	}
	else
	{
		device->filter_gain = 1.0;
	}

	airspyhf_set_freq(device, device->freq_hz);

	return AIRSPYHF_SUCCESS;
}

int ADDCALL airspyhf_set_receiver_mode(airspyhf_device_t *device, receiver_mode_t value)
{
	int result;
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		AIRSPYHF_RECEIVER_MODE,
		value,
		0,
		NULL,
		0,
		0);

	if (result != 0)
	{
		return AIRSPYHF_ERROR;
	}

	return AIRSPYHF_SUCCESS;
}

int ADDCALL airspyhf_start(airspyhf_device_t *device, airspyhf_sample_block_cb_fn callback, void *ctx)
{
	int result;

	memset(device->dropped_buffers_queue, 0, RAW_BUFFER_COUNT * sizeof(uint32_t));
	device->dropped_buffers = 0;

	device->vec.re = 1.0f;
	device->vec.im = 0.0f;

	result = airspyhf_set_receiver_mode(device, RECEIVER_MODE_OFF);
	if (result != AIRSPYHF_SUCCESS)
	{
		return result;
	}

	libusb_clear_halt(device->usb_device, LIBUSB_ENDPOINT_IN | AIRSPYHF_ENDPOINT_IN);

	result = airspyhf_set_receiver_mode(device, RECEIVER_MODE_ON);
	if (result == AIRSPYHF_SUCCESS)
	{
		device->ctx = ctx;
		result = create_io_threads(device, callback);
	}

	return result;
}

int ADDCALL airspyhf_is_streaming(airspyhf_device_t *device)
{
	return device->streaming;
}

int ADDCALL airspyhf_stop(airspyhf_device_t *device)
{
	int result1, result2;
	result1 = kill_io_threads(device);
	result2 = airspyhf_set_receiver_mode(device, RECEIVER_MODE_OFF);

	if (result2 != AIRSPYHF_SUCCESS)
	{
		return result2;
	}
	return result1;
}

int ADDCALL airspyhf_set_freq(airspyhf_device_t *device, const uint32_t freq_hz)
{
	const int tuning_alignment = 1000;

	int result;
	uint8_t buf[4];
	uint32_t lo_low_khz = device->is_low_if ? MIN_LOW_IF_LO : MIN_ZERO_IF_LO;
	uint32_t if_shift = (device->enable_dsp && !device->is_low_if) ? DEFAULT_IF_SHIFT : 0;
	uint32_t adjusted_freq_hz = (uint32_t)((int64_t)freq_hz * (int64_t)(1000000000LL + device->calibration_ppb) / 1000000000LL);
	uint32_t freq_khz = MAX(lo_low_khz, (adjusted_freq_hz + if_shift + tuning_alignment / 2) / tuning_alignment);

	if (device->freq_khz != freq_khz)
	{
		buf[0] = (uint8_t)((freq_khz >> 24) & 0xff);
		buf[1] = (uint8_t)((freq_khz >> 16) & 0xff);
		buf[2] = (uint8_t)((freq_khz >> 8) & 0xff);
		buf[3] = (uint8_t)((freq_khz)&0xff);

		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPYHF_SET_FREQ,
			0,
			0,
			(unsigned char *)&buf,
			sizeof(buf),
			0);

		iq_balancer_set_optimal_point(device->iq_balancer, device->optimal_point);

		if (result < sizeof(buf))
		{
			return AIRSPYHF_ERROR;
		}

		device->freq_khz = freq_khz;
	}

	device->freq_hz = freq_hz;
	device->freq_shift = adjusted_freq_hz - freq_khz * 1000;

	return AIRSPYHF_SUCCESS;
}

static int airspyhf_config_write(airspyhf_device_t *device, uint8_t *buffer, uint16_t length)
{
	int result;
	uint8_t buf[256];

	memset(buf, 0, sizeof(buf));

	memcpy(buf, buffer, MIN(length, sizeof(buf)));

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		AIRSPYHF_CONFIG_WRITE,
		0,
		0,
		buf,
		sizeof(buf),
		0);

	if (result < sizeof(buf))
	{
		return AIRSPYHF_ERROR;
	}

	return AIRSPYHF_SUCCESS;
}

static int airspyhf_config_read(airspyhf_device_t *device, uint8_t *buffer, uint16_t length)
{
	uint8_t buf[256];
	int result;

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		AIRSPYHF_CONFIG_READ,
		0,
		0,
		buf,
		sizeof(buf),
		0);

	memcpy(buffer, buf, MIN(length, sizeof(buf)));

	if (result < sizeof(buf))
	{
		return AIRSPYHF_ERROR;
	}

	return AIRSPYHF_SUCCESS;
}

int ADDCALL airspyhf_get_calibration(airspyhf_device_t *device, int32_t *ppb)
{
	if (ppb)
	{
		*ppb = device->calibration_ppb;
		return AIRSPYHF_SUCCESS;
	}

	return AIRSPYHF_ERROR;
}

int ADDCALL airspyhf_flash_calibration(airspyhf_device_t *device)
{
	calibration_record_t record;
	record.magic_number = CALIBRATION_MAGIC;
	record.calibration_ppb = device->calibration_ppb;

	if (airspyhf_is_streaming(device))
	{
		return AIRSPYHF_ERROR;
	}

	if (airspyhf_config_write(device, (uint8_t *)&record, sizeof(record)) != AIRSPYHF_SUCCESS)
	{
		return AIRSPYHF_ERROR;
	}

	return AIRSPYHF_SUCCESS;
}

int ADDCALL airspyhf_set_calibration(airspyhf_device_t *device, int32_t ppb)
{
	device->calibration_ppb = ppb;
	return airspyhf_set_freq(device, device->freq_hz);
}

int ADDCALL airspyhf_set_optimal_iq_correction_point(airspyhf_device_t *device, float w)
{
	device->optimal_point = w;
	iq_balancer_set_optimal_point(device->iq_balancer, device->optimal_point);
	return AIRSPYHF_SUCCESS;
}

int ADDCALL airspyhf_iq_balancer_configure(airspyhf_device_t *device, int buffers_to_skip, int fft_integration, int fft_overlap, int correlation_integration)
{
	iq_balancer_configure(device->iq_balancer, buffers_to_skip, fft_integration, fft_overlap, correlation_integration);
	return AIRSPYHF_SUCCESS;
}

int ADDCALL airspyhf_board_partid_serialno_read(airspyhf_device_t *device, airspyhf_read_partid_serialno_t *read_partid_serialno)
{
	uint8_t length;
	int result;

	length = sizeof(airspyhf_read_partid_serialno_t);
	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		AIRSPYHF_GET_SERIALNO_BOARDID,
		0,
		0,
		(unsigned char *)read_partid_serialno,
		length,
		0);

	if (result < length)
	{
		return AIRSPYHF_ERROR;
	}

	return AIRSPYHF_SUCCESS;
}

int ADDCALL airspyhf_version_string_read(airspyhf_device_t *device, char *version, uint8_t length)
{
	int result;
	char version_local[MAX_VERSION_STRING_SIZE];

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		AIRSPYHF_GET_VERSION_STRING,
		0,
		0,
		(unsigned char *)version_local,
		(MAX_VERSION_STRING_SIZE - 1),
		0);

	if (result < 0)
	{
		return AIRSPYHF_ERROR;
	}
	else
	{
		if (length > 0)
		{
			memcpy(version, version_local, length - 1);
			version[length - 1] = 0;
			return AIRSPYHF_SUCCESS;
		}
		else
		{
			return AIRSPYHF_ERROR;
		}
	}
}

int ADDCALL airspyhf_set_user_output(airspyhf_device_t *device, airspyhf_user_output_t pin, airspyhf_user_output_state_t value)
{
	int result;

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		AIRSPYHF_SET_USER_OUTPUT,
		(uint16_t)pin,
		(uint16_t)value,
		NULL,
		0,
		0);

	if (result < 0)
	{
		return AIRSPYHF_ERROR;
	}

	return AIRSPYHF_SUCCESS;
}

int ADDCALL airspyhf_set_hf_agc(airspyhf_device_t *device, uint8_t flag)
{
	int result;

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		AIRSPYHF_SET_HF_AGC,
		(uint16_t)flag,
		0,
		NULL,
		0,
		0);

	if (result < 0)
	{
		return AIRSPYHF_ERROR;
	}

	return AIRSPYHF_SUCCESS;
}

int ADDCALL airspyhf_set_hf_agc_threshold(airspyhf_device_t *device, uint8_t flag)
{
	int result;

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		AIRSPYHF_SET_HF_AGC_THRESHOLD,
		(uint16_t)flag,
		0,
		NULL,
		0,
		0);

	if (result < 0)
	{
		return AIRSPYHF_ERROR;
	}

	return AIRSPYHF_SUCCESS;
}

int ADDCALL airspyhf_set_hf_att(airspyhf_device_t *device, uint8_t value)
{
	int result;

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		AIRSPYHF_SET_HF_ATT,
		(uint16_t)value,
		0,
		NULL,
		0,
		0);

	if (result < 0)
	{
		return AIRSPYHF_ERROR;
	}

	return AIRSPYHF_SUCCESS;
}

int ADDCALL airspyhf_set_hf_lna(airspyhf_device_t *device, uint8_t flag)
{
	int result;

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		AIRSPYHF_SET_HF_LNA,
		(uint16_t)flag,
		0,
		NULL,
		0,
		0);

	if (result < 0)
	{
		return AIRSPYHF_ERROR;
	}

	return AIRSPYHF_SUCCESS;
}

int ADDCALL airspyhf_set_lib_dsp(airspyhf_device_t *device, const uint8_t flag)
{
	device->enable_dsp = flag;
	return AIRSPYHF_SUCCESS;
}

//#include "special.c"
