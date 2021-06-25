/*
Copyright (c) 2013, Michael Ossmann <mike@ossmann.com>
Copyright (c) 2012, Jared Boone <jared@sharebrained.com>
Copyright (c) 2014, Youssef Touil <youssef@airspy.com>
Copyright (c) 2014, Benjamin Vernoux <bvernoux@airspy.com>
Copyright (c) 2015, Ian Gilmour <ian@sdrsharp.com>

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

		Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
		Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the
		documentation and/or other materials provided with the distribution.
		Neither the name of AirSpy nor the names of its contributors may be used to endorse or promote products derived from this software
		without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb.h>
#include <pthread.h>

#include "airspy.h"
#include "iqconverter_float.h"
#include "iqconverter_int16.h"
#include "filters.h"

#ifndef bool
typedef int bool;
#define true 1
#define false 0
#endif

#define PACKET_SIZE (12)
#define UNPACKED_SIZE (16)
#define RAW_BUFFER_COUNT (8)

#ifdef AIRSPY_BIG_ENDIAN
#define TO_LE(x) __builtin_bswap32(x)
#else
#define TO_LE(x) x
#endif

#define SAMPLE_RESOLUTION 12
#define SAMPLE_ENCAPSULATION 16

#define SAMPLE_SHIFT (SAMPLE_ENCAPSULATION - SAMPLE_RESOLUTION)
#define SAMPLE_SCALE (1.0f / (1 << (15 - SAMPLE_SHIFT)))

#define SERIAL_NUMBER_UNUSED (0ULL)

#define USB_PRODUCT_ID (2)
#define STR_DESCRIPTOR_SIZE (250)

#define MIN_SAMPLERATE_BY_VALUE (1000000)
#define SAMPLE_TYPE_IS_IQ(x) ((x) == AIRSPY_SAMPLE_FLOAT32_IQ || (x) == AIRSPY_SAMPLE_INT16_IQ)

typedef struct
{
	uint32_t freq_hz;
} set_freq_params_t;

typedef struct airspy_device
{
	libusb_context *usb_context;
	libusb_device_handle *usb_device;
	struct libusb_transfer **transfers;
	airspy_sample_block_cb_fn callback;
	volatile bool streaming;
	volatile bool stop_requested;
	pthread_t transfer_thread;
	pthread_t consumer_thread;
	pthread_cond_t consumer_cv;
	pthread_mutex_t consumer_mp;
	uint32_t supported_samplerate_count;
	uint32_t *supported_samplerates;
	uint32_t transfer_count;
	uint32_t buffer_size;
	uint32_t dropped_buffers;
	uint32_t dropped_buffers_queue[RAW_BUFFER_COUNT];
	uint16_t *received_samples_queue[RAW_BUFFER_COUNT];
	volatile int received_samples_queue_head;
	volatile int received_samples_queue_tail;
	volatile int received_buffer_count;
	void *output_buffer;
	uint16_t *unpacked_samples;
	bool packing_enabled;
	iqconverter_float_t *cnv_f;
	iqconverter_int16_t *cnv_i;
	void *ctx;
	enum airspy_sample_type sample_type;
} airspy_device_t;

static const uint16_t airspy_usb_vid = 0x1d50;
static const uint16_t airspy_usb_pid = 0x60a1;

#define STR_PREFIX_SERIAL_AIRSPY_SIZE (10)

#define SERIAL_AIRSPY_EXPECTED_SIZE (26)

#define GAIN_COUNT (22)

uint8_t airspy_linearity_vga_gains[GAIN_COUNT] = {13, 12, 11, 11, 11, 11, 11, 10, 10, 10, 10, 10, 10, 10, 10, 10, 9, 8, 7, 6, 5, 4};
uint8_t airspy_linearity_mixer_gains[GAIN_COUNT] = {12, 12, 11, 9, 8, 7, 6, 6, 5, 0, 0, 1, 0, 0, 2, 2, 1, 1, 1, 1, 0, 0};
uint8_t airspy_linearity_lna_gains[GAIN_COUNT] = {14, 14, 14, 13, 12, 10, 9, 9, 8, 9, 8, 6, 5, 3, 1, 0, 0, 0, 0, 0, 0, 0};
uint8_t airspy_sensitivity_vga_gains[GAIN_COUNT] = {13, 12, 11, 10, 9, 8, 7, 6, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4};
uint8_t airspy_sensitivity_mixer_gains[GAIN_COUNT] = {12, 12, 12, 12, 11, 10, 10, 9, 9, 8, 7, 4, 4, 4, 3, 2, 2, 1, 0, 0, 0, 0};
uint8_t airspy_sensitivity_lna_gains[GAIN_COUNT] = {14, 14, 14, 14, 14, 14, 14, 14, 14, 13, 12, 12, 9, 9, 8, 7, 6, 5, 3, 2, 1, 0};

static int cancel_transfers(airspy_device_t *device)
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
		return AIRSPY_SUCCESS;
	}
	else
	{
		return AIRSPY_ERROR_OTHER;
	}
}

static int free_transfers(airspy_device_t *device)
{
	int i;
	uint32_t transfer_index;

	if (device->transfers != NULL)
	{
		// libusb_close() should free all transfers referenced from this array.
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

		if (device->output_buffer != NULL)
		{
			free(device->output_buffer);
			device->output_buffer = NULL;
		}

		if (device->unpacked_samples != NULL)
		{
			free(device->unpacked_samples);
			device->unpacked_samples = NULL;
		}

		for (i = 0; i < RAW_BUFFER_COUNT; i++)
		{
			if (device->received_samples_queue[i] != NULL)
			{
				free(device->received_samples_queue[i]);
				device->received_samples_queue[i] = NULL;
			}
		}
	}

	return AIRSPY_SUCCESS;
}

static int allocate_transfers(airspy_device_t *const device)
{
	int i;
	size_t sample_count;
	uint32_t transfer_index;

	if (device->transfers == NULL)
	{
		for (i = 0; i < RAW_BUFFER_COUNT; i++)
		{
			device->received_samples_queue[i] = (uint16_t *)malloc(device->buffer_size);
			if (device->received_samples_queue[i] == NULL)
			{
				return AIRSPY_ERROR_NO_MEM;
			}

			memset(device->received_samples_queue[i], 0, device->buffer_size);
		}

		if (device->packing_enabled)
		{
			sample_count = ((device->buffer_size / 2) * 4) / 3;
		}
		else
		{
			sample_count = device->buffer_size / 2;
		}

		device->output_buffer = (float *)malloc(sample_count * sizeof(float));
		if (device->output_buffer == NULL)
		{
			return AIRSPY_ERROR_NO_MEM;
		}

		if (device->packing_enabled)
		{
			device->unpacked_samples = (uint16_t *)malloc(sample_count * sizeof(uint16_t));
			if (device->unpacked_samples == NULL)
			{
				return AIRSPY_ERROR_NO_MEM;
			}
		}

		device->transfers = (struct libusb_transfer **)calloc(device->transfer_count, sizeof(struct libusb_transfer));
		if (device->transfers == NULL)
		{
			return AIRSPY_ERROR_NO_MEM;
		}

		for (transfer_index = 0; transfer_index < device->transfer_count; transfer_index++)
		{
			device->transfers[transfer_index] = libusb_alloc_transfer(0);
			if (device->transfers[transfer_index] == NULL)
			{
				return AIRSPY_ERROR_LIBUSB;
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
				return AIRSPY_ERROR_NO_MEM;
			}
		}
		return AIRSPY_SUCCESS;
	}
	else
	{
		return AIRSPY_ERROR_BUSY;
	}
}

static int prepare_transfers(airspy_device_t *device, const uint_fast8_t endpoint_address, libusb_transfer_cb_fn callback)
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
				return AIRSPY_ERROR_LIBUSB;
			}
		}
		return AIRSPY_SUCCESS;
	}
	else
	{
		// This shouldn't happen.
		return AIRSPY_ERROR_OTHER;
	}
}

static void convert_samples_int16(uint16_t *src, int16_t *dest, int count)
{
	int i;
	for (i = 0; i < count; i += 4)
	{
		dest[i + 0] = (src[i + 0] - 2048) << SAMPLE_SHIFT;
		dest[i + 1] = (src[i + 1] - 2048) << SAMPLE_SHIFT;
		dest[i + 2] = (src[i + 2] - 2048) << SAMPLE_SHIFT;
		dest[i + 3] = (src[i + 3] - 2048) << SAMPLE_SHIFT;
	}
}

static void convert_samples_float(uint16_t *src, float *dest, int count)
{
	int i;
	for (i = 0; i < count; i += 4)
	{
		dest[i + 0] = (src[i + 0] - 2048) * SAMPLE_SCALE;
		dest[i + 1] = (src[i + 1] - 2048) * SAMPLE_SCALE;
		dest[i + 2] = (src[i + 2] - 2048) * SAMPLE_SCALE;
		dest[i + 3] = (src[i + 3] - 2048) * SAMPLE_SCALE;
	}
}

static inline void unpack_samples(uint32_t *input, uint16_t *output, int length)
{
	int i, j;

	for (i = 0, j = 0; j < length; i += 3, j += 8)
	{
		output[j + 0] = (input[i] >> 20) & 0xfff;
		output[j + 1] = (input[i] >> 8) & 0xfff;
		output[j + 2] = ((input[i] & 0xff) << 4) | ((input[i + 1] >> 28) & 0xf);
		output[j + 3] = ((input[i + 1] & 0xfff0000) >> 16);
		output[j + 4] = ((input[i + 1] & 0xfff0) >> 4);
		output[j + 5] = ((input[i + 1] & 0xf) << 8) | ((input[i + 2] & 0xff000000) >> 24);
		output[j + 6] = ((input[i + 2] >> 12) & 0xfff);
		output[j + 7] = ((input[i + 2] & 0xfff));
	}
}

static void *consumer_threadproc(void *arg)
{
	int sample_count;
	uint16_t *input_samples;
	uint32_t dropped_buffers;
	airspy_device_t *device = (airspy_device_t *)arg;
	airspy_transfer_t transfer;

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

		input_samples = device->received_samples_queue[device->received_samples_queue_tail];
		dropped_buffers = device->dropped_buffers_queue[device->received_samples_queue_tail];
		device->received_samples_queue_tail = (device->received_samples_queue_tail + 1) & (RAW_BUFFER_COUNT - 1);

		pthread_mutex_unlock(&device->consumer_mp);

		if (device->packing_enabled)
		{
			sample_count = ((device->buffer_size / 2) * 4) / 3;

			if (device->sample_type != AIRSPY_SAMPLE_RAW)
			{
				unpack_samples((uint32_t *)input_samples, device->unpacked_samples, sample_count);

				input_samples = device->unpacked_samples;
			}
		}
		else
		{
			sample_count = device->buffer_size / 2;
		}

		switch (device->sample_type)
		{
		case AIRSPY_SAMPLE_FLOAT32_IQ:
			convert_samples_float(input_samples, (float *)device->output_buffer, sample_count);
			iqconverter_float_process(device->cnv_f, (float *)device->output_buffer, sample_count);
			sample_count /= 2;
			transfer.samples = device->output_buffer;
			break;

		case AIRSPY_SAMPLE_FLOAT32_REAL:
			convert_samples_float(input_samples, (float *)device->output_buffer, sample_count);
			transfer.samples = device->output_buffer;
			break;

		case AIRSPY_SAMPLE_INT16_IQ:
			convert_samples_int16(input_samples, (int16_t *)device->output_buffer, sample_count);
			iqconverter_int16_process(device->cnv_i, (int16_t *)device->output_buffer, sample_count);
			sample_count /= 2;
			transfer.samples = device->output_buffer;
			break;

		case AIRSPY_SAMPLE_INT16_REAL:
			convert_samples_int16(input_samples, (int16_t *)device->output_buffer, sample_count);
			transfer.samples = device->output_buffer;
			break;

		case AIRSPY_SAMPLE_UINT16_REAL:
		case AIRSPY_SAMPLE_RAW:
			transfer.samples = input_samples;
			break;

		case AIRSPY_SAMPLE_END:
			// Just to shut GCC's moaning
			break;
		}

		transfer.device = device;
		transfer.ctx = device->ctx;
		transfer.sample_count = sample_count;
		transfer.sample_type = device->sample_type;
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

static void airspy_libusb_transfer_callback(struct libusb_transfer *usb_transfer)
{
	uint16_t *temp;
	airspy_device_t *device = (airspy_device_t *)usb_transfer->user_data;

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
			device->received_samples_queue[device->received_samples_queue_head] = (uint16_t *)usb_transfer->buffer;
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
			device->stop_requested = true;
		}
	}
	else
	{
		device->stop_requested = true;
	}
}

static void *transfer_threadproc(void *arg)
{
	airspy_device_t *device = (airspy_device_t *)arg;
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
				device->stop_requested = true;
		}
	}

	pthread_exit(NULL);

	return NULL;
}

static int kill_io_threads(airspy_device_t *device)
{
	struct timeval timeout = {0, 0};

	if (device->streaming)
	{
		device->stop_requested = true;
		cancel_transfers(device);

		pthread_mutex_lock(&device->consumer_mp);
		pthread_cond_signal(&device->consumer_cv);
		pthread_mutex_unlock(&device->consumer_mp);

		pthread_join(device->transfer_thread, NULL);
		pthread_join(device->consumer_thread, NULL);

		libusb_handle_events_timeout_completed(device->usb_context, &timeout, NULL);

		device->stop_requested = false;
		device->streaming = false;
	}

	return AIRSPY_SUCCESS;
}

static int create_io_threads(airspy_device_t *device, airspy_sample_block_cb_fn callback)
{
	int result;
	pthread_attr_t attr;

	if (!device->streaming && !device->stop_requested)
	{
		device->callback = callback;
		device->streaming = true;

		result = prepare_transfers(device, LIBUSB_ENDPOINT_IN | 1, (libusb_transfer_cb_fn)airspy_libusb_transfer_callback);
		if (result != AIRSPY_SUCCESS)
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
			return AIRSPY_ERROR_THREAD;
		}

		result = pthread_create(&device->transfer_thread, &attr, transfer_threadproc, device);
		if (result != 0)
		{
			return AIRSPY_ERROR_THREAD;
		}

		pthread_attr_destroy(&attr);
	}
	else
	{
		return AIRSPY_ERROR_BUSY;
	}

	return AIRSPY_SUCCESS;
}

static void airspy_open_exit(airspy_device_t *device)
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

static void airspy_open_device(airspy_device_t *device,
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
	unsigned char serial_number[SERIAL_AIRSPY_EXPECTED_SIZE + 1];

	libusb_dev_handle = &device->usb_device;
	*libusb_dev_handle = NULL;

	cnt = libusb_get_device_list(device->usb_context, &devices);
	if (cnt < 0)
	{
		*ret = AIRSPY_ERROR_NOT_FOUND;
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
					if (serial_number_len == SERIAL_AIRSPY_EXPECTED_SIZE)
					{
						uint64_t serial = 0;
						// use same code to determine device's serial number as in airspy_list_devices()
						{
							char *start, *end;

							serial_number[SERIAL_AIRSPY_EXPECTED_SIZE] = 0;
							start = (char *)(serial_number + STR_PREFIX_SERIAL_AIRSPY_SIZE);
							end = NULL;
							serial = strtoull(start, &end, 16);
						}

						if (serial == serial_number_val)
						{
#ifdef __linux__
							/* Check whether a kernel driver is attached to interface #0. If so, we'll
							* need to detach it.
							*/
							if (libusb_kernel_driver_active(dev_handle, 0))
							{
								libusb_detach_kernel_driver(dev_handle, 0);
							}
#endif
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
#ifdef __linux__
					/* Check whether a kernel driver is attached to interface #0. If so, we'll
					* need to detach it.
					*/
					if (libusb_kernel_driver_active(dev_handle, 0))
					{
						libusb_detach_kernel_driver(dev_handle, 0);
					}
#endif
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
					break;
				}
			}
		}
	}
	libusb_free_device_list(devices, 1);

	dev_handle = device->usb_device;
	if (dev_handle == NULL)
	{
		*ret = AIRSPY_ERROR_NOT_FOUND;
		return;
	}

	*ret = AIRSPY_SUCCESS;
	return;
}

static int airspy_read_samplerates_from_fw(struct airspy_device *device, uint32_t *buffer, const uint32_t len)
{
	int result;

	result = libusb_control_transfer(
		device->usb_device,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		AIRSPY_GET_SAMPLERATES,
		0,
		len,
		(unsigned char *)buffer,
		(len > 0 ? len : 1) * sizeof(uint32_t),
		0);

	if (result < 1)
	{
		return AIRSPY_ERROR_OTHER;
	}

	return AIRSPY_SUCCESS;
}

static int airspy_open_init(airspy_device_t **device, uint64_t serial_number)
{
	airspy_device_t *lib_device;
	int libusb_error;
	int result;

	*device = NULL;

	lib_device = (airspy_device_t *)calloc(1, sizeof(airspy_device_t));
	if (lib_device == NULL)
	{
		return AIRSPY_ERROR_NO_MEM;
	}

	libusb_error = libusb_init(&lib_device->usb_context);
	if (libusb_error != 0)
	{
		free(lib_device);
		return AIRSPY_ERROR_LIBUSB;
	}

	airspy_open_device(lib_device,
					   &result,
					   airspy_usb_vid,
					   airspy_usb_pid,
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
	lib_device->buffer_size = 262144;
	lib_device->packing_enabled = false;
	lib_device->streaming = false;
	lib_device->stop_requested = false;
	lib_device->sample_type = AIRSPY_SAMPLE_FLOAT32_IQ;

	result = airspy_read_samplerates_from_fw(lib_device, &lib_device->supported_samplerate_count, 0);
	if (result == AIRSPY_SUCCESS)
	{
		lib_device->supported_samplerates = (uint32_t *)malloc(lib_device->supported_samplerate_count * sizeof(uint32_t));
		result = airspy_read_samplerates_from_fw(lib_device, lib_device->supported_samplerates, lib_device->supported_samplerate_count);
		if (result != AIRSPY_SUCCESS)
		{
			free(lib_device->supported_samplerates);
		}
	}

	if (result != AIRSPY_SUCCESS)
	{
		lib_device->supported_samplerate_count = 2;
		lib_device->supported_samplerates = (uint32_t *)malloc(lib_device->supported_samplerate_count * sizeof(uint32_t));
		lib_device->supported_samplerates[0] = 10000000;
		lib_device->supported_samplerates[1] = 2500000;
	}

	airspy_set_packing(lib_device, 0);

	result = allocate_transfers(lib_device);
	if (result != 0)
	{
		airspy_open_exit(lib_device);
		free(lib_device->supported_samplerates);
		free(lib_device);
		return AIRSPY_ERROR_NO_MEM;
	}

	lib_device->cnv_f = iqconverter_float_create(HB_KERNEL_FLOAT, HB_KERNEL_FLOAT_LEN);
	lib_device->cnv_i = iqconverter_int16_create(HB_KERNEL_INT16, HB_KERNEL_INT16_LEN);

	pthread_cond_init(&lib_device->consumer_cv, NULL);
	pthread_mutex_init(&lib_device->consumer_mp, NULL);

	*device = lib_device;

	return AIRSPY_SUCCESS;
}

static void airspy_open2_device(airspy_device_t *device,
								int *ret,
								int fd,
								const char *devicePath)
{
	int result;
	libusb_device_handle **libusb_dev_handle;
	int serial_number_len;
	libusb_device_handle *dev_handle;
	libusb_device *dev;

	ssize_t cnt;
	int serial_descriptor_index;
	struct libusb_device_descriptor device_descriptor;
	unsigned char serial_number[SERIAL_AIRSPY_EXPECTED_SIZE + 1];

	libusb_dev_handle = &device->usb_device;
	*libusb_dev_handle = NULL;

	dev = libusb_get_device2(device->usb_context, devicePath);

	if (!dev)
	{
		*ret = AIRSPY_ERROR_NOT_FOUND;
		return;
	}

	if (libusb_open2(dev, libusb_dev_handle, fd) == 0)
	{
		dev_handle = *libusb_dev_handle;
#ifdef __linux__
		/* Check whether a kernel driver is attached to interface #0. If so, we'll
					* need to detach it.
					*/
		if (libusb_kernel_driver_active(dev_handle, 0))
		{
			libusb_detach_kernel_driver(dev_handle, 0);
		}
#endif
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
	}

	dev_handle = device->usb_device;
	if (dev_handle == NULL)
	{
		*ret = AIRSPY_ERROR_NOT_FOUND;
		return;
	}

	*ret = AIRSPY_SUCCESS;
	return;
}

#ifdef __cplusplus
extern "C"
{
#endif

	int ADDCALL airspy_open2(struct airspy_device **device, int fd, const char *devicePath)
	{
		airspy_device_t *lib_device;
		int libusb_error;
		int result;

		*device = NULL;

		lib_device = (airspy_device_t *)calloc(1, sizeof(airspy_device_t));
		if (lib_device == NULL)
		{
			return AIRSPY_ERROR_NO_MEM;
		}

		libusb_error = libusb_init(&lib_device->usb_context);
		if (libusb_error != 0)
		{
			free(lib_device);
			return AIRSPY_ERROR_LIBUSB;
		}

		airspy_open2_device(lib_device,
							&result,
							fd,
							devicePath);
		if (lib_device->usb_device == NULL)
		{
			libusb_exit(lib_device->usb_context);
			free(lib_device);
			return result;
		}

		lib_device->transfers = NULL;
		lib_device->callback = NULL;
		lib_device->transfer_count = 16;
		lib_device->buffer_size = 262144;
		lib_device->packing_enabled = false;
		lib_device->streaming = false;
		lib_device->stop_requested = false;
		lib_device->sample_type = AIRSPY_SAMPLE_FLOAT32_IQ;

		result = airspy_read_samplerates_from_fw(lib_device, &lib_device->supported_samplerate_count, 0);
		if (result == AIRSPY_SUCCESS)
		{
			lib_device->supported_samplerates = (uint32_t *)malloc(lib_device->supported_samplerate_count * sizeof(uint32_t));
			result = airspy_read_samplerates_from_fw(lib_device, lib_device->supported_samplerates, lib_device->supported_samplerate_count);
			if (result != AIRSPY_SUCCESS)
			{
				free(lib_device->supported_samplerates);
			}
		}

		if (result != AIRSPY_SUCCESS)
		{
			lib_device->supported_samplerate_count = 2;
			lib_device->supported_samplerates = (uint32_t *)malloc(lib_device->supported_samplerate_count * sizeof(uint32_t));
			lib_device->supported_samplerates[0] = 10000000;
			lib_device->supported_samplerates[1] = 2500000;
		}

		airspy_set_packing(lib_device, 0);

		result = allocate_transfers(lib_device);
		if (result != 0)
		{
			airspy_open_exit(lib_device);
			free(lib_device->supported_samplerates);
			free(lib_device);
			return AIRSPY_ERROR_NO_MEM;
		}

		lib_device->cnv_f = iqconverter_float_create(HB_KERNEL_FLOAT, HB_KERNEL_FLOAT_LEN);
		lib_device->cnv_i = iqconverter_int16_create(HB_KERNEL_INT16, HB_KERNEL_INT16_LEN);

		pthread_cond_init(&lib_device->consumer_cv, NULL);
		pthread_mutex_init(&lib_device->consumer_mp, NULL);

		*device = lib_device;

		return AIRSPY_SUCCESS;
	}

	void ADDCALL airspy_lib_version(airspy_lib_version_t *lib_version)
	{
		lib_version->major_version = AIRSPY_VER_MAJOR;
		lib_version->minor_version = AIRSPY_VER_MINOR;
		lib_version->revision = AIRSPY_VER_REVISION;
	}

	/* airspy_init() deprecated */
	int ADDCALL airspy_init(void)
	{
		return AIRSPY_SUCCESS;
	}

	/* airspy_exit() deprecated */
	int ADDCALL airspy_exit(void)
	{
		return AIRSPY_SUCCESS;
	}

	int airspy_list_devices(uint64_t *serials, int count)
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
		unsigned char serial_number[SERIAL_AIRSPY_EXPECTED_SIZE + 1];

		if (serials)
		{
			memset(serials, 0, sizeof(uint64_t) * count);
		}

		if (libusb_init(&context) != 0)
		{
			return AIRSPY_ERROR_LIBUSB;
		}

		if (libusb_get_device_list(context, &devices) < 0)
		{
			return AIRSPY_ERROR_NOT_FOUND;
		}

		i = 0;
		output_count = 0;
		while ((dev = devices[i++]) != NULL && (!serials || output_count < count))
		{
			libusb_get_device_descriptor(dev, &device_descriptor);

			if ((device_descriptor.idVendor == airspy_usb_vid) &&
				(device_descriptor.idProduct == airspy_usb_pid))
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

					if (serial_number_len == SERIAL_AIRSPY_EXPECTED_SIZE)
					{
						char *start, *end;
						uint64_t serial;

						serial_number[SERIAL_AIRSPY_EXPECTED_SIZE] = 0;
						start = (char *)(serial_number + STR_PREFIX_SERIAL_AIRSPY_SIZE);
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

	int ADDCALL airspy_open_sn(airspy_device_t **device, uint64_t serial_number)
	{
		int result;

		result = airspy_open_init(device, serial_number);
		return result;
	}

	int ADDCALL airspy_open(airspy_device_t **device)
	{
		int result;

		result = airspy_open_init(device, SERIAL_NUMBER_UNUSED);
		return result;
	}

	int ADDCALL airspy_close(airspy_device_t *device)
	{
		int result;

		result = AIRSPY_SUCCESS;

		if (device != NULL)
		{
			result = airspy_stop_rx(device);

			iqconverter_float_free(device->cnv_f);
			iqconverter_int16_free(device->cnv_i);

			pthread_cond_destroy(&device->consumer_cv);
			pthread_mutex_destroy(&device->consumer_mp);

			airspy_open_exit(device);
			free_transfers(device);
			free(device->supported_samplerates);
			free(device);
		}

		return result;
	}

	int ADDCALL airspy_get_samplerates(struct airspy_device *device, uint32_t *buffer, const uint32_t len)
	{
		uint32_t i;

		if (len == 0)
		{
			*buffer = device->supported_samplerate_count;
		}
		else if (len <= device->supported_samplerate_count)
		{
			memcpy(buffer, device->supported_samplerates, len * sizeof(uint32_t));

			if (!SAMPLE_TYPE_IS_IQ(device->sample_type))
			{
				for (i = 0; i < len; i++)
				{
					buffer[i] *= 2;
				}
			}
		}
		else
		{
			return AIRSPY_ERROR_INVALID_PARAM;
		}

		return AIRSPY_SUCCESS;
	}

	int ADDCALL airspy_set_samplerate(airspy_device_t *device, uint32_t samplerate)
	{
		int result;
		uint8_t retval;
		uint8_t length;
		uint32_t i;

		if (samplerate >= MIN_SAMPLERATE_BY_VALUE)
		{
			for (i = 0; i < device->supported_samplerate_count; i++)
			{
				if (samplerate == device->supported_samplerates[i])
				{
					samplerate = i;
					break;
				}
			}

			if (samplerate >= MIN_SAMPLERATE_BY_VALUE)
			{
				if (SAMPLE_TYPE_IS_IQ(device->sample_type))
				{
					samplerate *= 2;
				}
				samplerate /= 1000;
			}
		}

		libusb_clear_halt(device->usb_device, LIBUSB_ENDPOINT_IN | 1);

		length = 1;

		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_SET_SAMPLERATE,
			0,
			samplerate,
			&retval,
			length,
			0);

		if (result < length)
		{
			return AIRSPY_ERROR_LIBUSB;
		}
		else
		{
			return AIRSPY_SUCCESS;
		}
	}

	int ADDCALL airspy_set_receiver_mode(airspy_device_t *device, receiver_mode_t value)
	{
		int result;
		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_RECEIVER_MODE,
			value,
			0,
			NULL,
			0,
			0);

		if (result != 0)
		{
			return AIRSPY_ERROR_LIBUSB;
		}
		else
		{
			return AIRSPY_SUCCESS;
		}
	}

	int ADDCALL airspy_start_rx(airspy_device_t *device, airspy_sample_block_cb_fn callback, void *ctx)
	{
		int result;

		iqconverter_float_reset(device->cnv_f);
		iqconverter_int16_reset(device->cnv_i);

		memset(device->dropped_buffers_queue, 0, RAW_BUFFER_COUNT * sizeof(uint32_t));
		device->dropped_buffers = 0;

		result = airspy_set_receiver_mode(device, RECEIVER_MODE_OFF);
		if (result != AIRSPY_SUCCESS)
		{
			return result;
		}

		libusb_clear_halt(device->usb_device, LIBUSB_ENDPOINT_IN | 1);

		result = airspy_set_receiver_mode(device, RECEIVER_MODE_RX);
		if (result == AIRSPY_SUCCESS)
		{
			device->ctx = ctx;
			result = create_io_threads(device, callback);
		}

		return result;
	}

	int ADDCALL airspy_stop_rx(airspy_device_t *device)
	{
		int result1, result2;
		result1 = kill_io_threads(device);

		result2 = airspy_set_receiver_mode(device, RECEIVER_MODE_OFF);
		if (result2 != AIRSPY_SUCCESS)
		{
			return result2;
		}
		return result1;
	}

	int ADDCALL airspy_si5351c_read(airspy_device_t *device, uint8_t register_number, uint8_t *value)
	{
		uint8_t temp_value;
		int result;

		temp_value = 0;
		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_SI5351C_READ,
			0,
			register_number,
			(unsigned char *)&temp_value,
			1,
			0);

		if (result < 1)
		{
			return AIRSPY_ERROR_LIBUSB;
		}
		else
		{
			*value = temp_value;
			return AIRSPY_SUCCESS;
		}
	}

	int ADDCALL airspy_si5351c_write(airspy_device_t *device, uint8_t register_number, uint8_t value)
	{
		int result;

		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_SI5351C_WRITE,
			value,
			register_number,
			NULL,
			0,
			0);

		if (result != 0)
		{
			return AIRSPY_ERROR_LIBUSB;
		}
		else
		{
			return AIRSPY_SUCCESS;
		}
	}

	int ADDCALL airspy_r820t_read(airspy_device_t *device, uint8_t register_number, uint8_t *value)
	{
		int result;

		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_R820T_READ,
			0,
			register_number,
			(unsigned char *)value,
			1,
			0);

		if (result < 1)
		{
			return AIRSPY_ERROR_LIBUSB;
		}
		else
		{
			return AIRSPY_SUCCESS;
		}
	}

	int ADDCALL airspy_r820t_write(airspy_device_t *device, uint8_t register_number, uint8_t value)
	{
		int result;

		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_R820T_WRITE,
			value,
			register_number,
			NULL,
			0,
			0);

		if (result != 0)
		{
			return AIRSPY_ERROR_LIBUSB;
		}
		else
		{
			return AIRSPY_SUCCESS;
		}
	}

	int ADDCALL airspy_gpio_read(airspy_device_t *device, airspy_gpio_port_t port, airspy_gpio_pin_t pin, uint8_t *value)
	{
		int result;
		uint8_t port_pin;

		port_pin = ((uint8_t)port) << 5;
		port_pin = port_pin | pin;

		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_GPIO_READ,
			0,
			port_pin,
			(unsigned char *)value,
			1,
			0);

		if (result < 1)
		{
			return AIRSPY_ERROR_LIBUSB;
		}
		else
		{
			return AIRSPY_SUCCESS;
		}
	}

	int ADDCALL airspy_gpio_write(airspy_device_t *device, airspy_gpio_port_t port, airspy_gpio_pin_t pin, uint8_t value)
	{
		int result;
		uint8_t port_pin;

		port_pin = ((uint8_t)port) << 5;
		port_pin = port_pin | pin;

		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_GPIO_WRITE,
			value,
			port_pin,
			NULL,
			0,
			0);

		if (result != 0)
		{
			return AIRSPY_ERROR_LIBUSB;
		}
		else
		{
			return AIRSPY_SUCCESS;
		}
	}

	int ADDCALL airspy_gpiodir_read(airspy_device_t *device, airspy_gpio_port_t port, airspy_gpio_pin_t pin, uint8_t *value)
	{
		int result;
		uint8_t port_pin;

		port_pin = ((uint8_t)port) << 5;
		port_pin = port_pin | pin;

		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_GPIODIR_READ,
			0,
			port_pin,
			(unsigned char *)value,
			1,
			0);

		if (result < 1)
		{
			return AIRSPY_ERROR_LIBUSB;
		}
		else
		{
			return AIRSPY_SUCCESS;
		}
	}

	int ADDCALL airspy_gpiodir_write(airspy_device_t *device, airspy_gpio_port_t port, airspy_gpio_pin_t pin, uint8_t value)
	{
		int result;
		uint8_t port_pin;

		port_pin = ((uint8_t)port) << 5;
		port_pin = port_pin | pin;

		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_GPIODIR_WRITE,
			value,
			port_pin,
			NULL,
			0,
			0);

		if (result != 0)
		{
			return AIRSPY_ERROR_LIBUSB;
		}
		else
		{
			return AIRSPY_SUCCESS;
		}
	}

	int ADDCALL airspy_spiflash_erase(airspy_device_t *device)
	{
		int result;
		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_SPIFLASH_ERASE,
			0,
			0,
			NULL,
			0,
			0);

		if (result != 0)
		{
			return AIRSPY_ERROR_LIBUSB;
		}
		else
		{
			return AIRSPY_SUCCESS;
		}
	}

	int ADDCALL airspy_spiflash_erase_sector(airspy_device_t *device, const uint16_t sector_num)
	{
		int result;
		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_SPIFLASH_ERASE_SECTOR,
			sector_num,
			0,
			NULL,
			0,
			0);

		if (result != 0)
		{
			return AIRSPY_ERROR_LIBUSB;
		}
		else
		{
			return AIRSPY_SUCCESS;
		}
	}

	int ADDCALL airspy_spiflash_write(airspy_device_t *device, const uint32_t address, const uint16_t length, unsigned char *const data)
	{
		int result;

		if (address > 0x0FFFFF)
		{
			return AIRSPY_ERROR_INVALID_PARAM;
		}

		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_SPIFLASH_WRITE,
			address >> 16,
			address & 0xFFFF,
			data,
			length,
			0);

		if (result < length)
		{
			return AIRSPY_ERROR_LIBUSB;
		}
		else
		{
			return AIRSPY_SUCCESS;
		}
	}

	int ADDCALL airspy_spiflash_read(airspy_device_t *device, const uint32_t address, const uint16_t length, unsigned char *data)
	{
		int result;

		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_SPIFLASH_READ,
			address >> 16,
			address & 0xFFFF,
			data,
			length,
			0);

		if (result < length)
		{
			return AIRSPY_ERROR_LIBUSB;
		}
		else
		{
			return AIRSPY_SUCCESS;
		}
	}

	int ADDCALL airspy_board_id_read(airspy_device_t *device, uint8_t *value)
	{
		int result;
		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_BOARD_ID_READ,
			0,
			0,
			value,
			1,
			0);

		if (result < 1)
		{
			return AIRSPY_ERROR_LIBUSB;
		}
		else
		{
			return AIRSPY_SUCCESS;
		}
	}

	int ADDCALL airspy_version_string_read(airspy_device_t *device, char *version, uint8_t length)
	{
#define VERSION_LOCAL_SIZE (128)
		int result;
		char version_local[VERSION_LOCAL_SIZE];

		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_VERSION_STRING_READ,
			0,
			0,
			(unsigned char *)version_local,
			(VERSION_LOCAL_SIZE - 1),
			0);

		if (result < 0)
		{
			return AIRSPY_ERROR_LIBUSB;
		}
		else
		{
			if (length > 0)
			{
				memcpy(version, version_local, length - 1);
				version[length - 1] = 0;
				return AIRSPY_SUCCESS;
			}
			else
			{
				return AIRSPY_ERROR_INVALID_PARAM;
			}
		}
	}

	int ADDCALL airspy_board_partid_serialno_read(airspy_device_t *device, airspy_read_partid_serialno_t *read_partid_serialno)
	{
		uint8_t length;
		int result;

		length = sizeof(airspy_read_partid_serialno_t);
		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_BOARD_PARTID_SERIALNO_READ,
			0,
			0,
			(unsigned char *)read_partid_serialno,
			length,
			0);

		if (result < length)
		{
			return AIRSPY_ERROR_LIBUSB;
		}
		else
		{

			read_partid_serialno->part_id[0] = TO_LE(read_partid_serialno->part_id[0]);
			read_partid_serialno->part_id[1] = TO_LE(read_partid_serialno->part_id[1]);
			read_partid_serialno->serial_no[0] = TO_LE(read_partid_serialno->serial_no[0]);
			read_partid_serialno->serial_no[1] = TO_LE(read_partid_serialno->serial_no[1]);
			read_partid_serialno->serial_no[2] = TO_LE(read_partid_serialno->serial_no[2]);
			read_partid_serialno->serial_no[3] = TO_LE(read_partid_serialno->serial_no[3]);

			return AIRSPY_SUCCESS;
		}
	}

	int ADDCALL airspy_set_sample_type(struct airspy_device *device, enum airspy_sample_type sample_type)
	{
		device->sample_type = sample_type;
		return AIRSPY_SUCCESS;
	}

	int ADDCALL airspy_set_freq(airspy_device_t *device, const uint32_t freq_hz)
	{
		set_freq_params_t set_freq_params;
		uint8_t length;
		int result;

		set_freq_params.freq_hz = TO_LE(freq_hz);
		length = sizeof(set_freq_params_t);

		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_SET_FREQ,
			0,
			0,
			(unsigned char *)&set_freq_params,
			length,
			0);

		if (result < length)
		{
			return AIRSPY_ERROR_LIBUSB;
		}
		else
		{
			return AIRSPY_SUCCESS;
		}
	}

	int ADDCALL airspy_set_conversion_filter_float32(struct airspy_device *device, const float *kernel, const uint32_t len)
	{
		if (device->streaming)
		{
			return AIRSPY_ERROR_BUSY;
		}

		iqconverter_float_free(device->cnv_f);
		device->cnv_f = iqconverter_float_create(kernel, len);

		return AIRSPY_SUCCESS;
	}

	int ADDCALL airspy_set_conversion_filter_int16(struct airspy_device *device, const int16_t *kernel, const uint32_t len)
	{
		if (device->streaming)
		{
			return AIRSPY_ERROR_BUSY;
		}

		iqconverter_int16_free(device->cnv_i);
		device->cnv_i = iqconverter_int16_create(kernel, len);

		return AIRSPY_SUCCESS;
	}

	int ADDCALL airspy_set_lna_gain(airspy_device_t *device, uint8_t value)
	{
		int result;
		uint8_t retval;
		uint8_t length;

		if (value > 14)
			value = 14;

		length = 1;

		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_SET_LNA_GAIN,
			0,
			value,
			&retval,
			length,
			0);

		if (result < length)
		{
			return AIRSPY_ERROR_LIBUSB;
		}
		else
		{
			return AIRSPY_SUCCESS;
		}
	}

	int ADDCALL airspy_set_mixer_gain(airspy_device_t *device, uint8_t value)
	{
		int result;
		uint8_t retval;
		uint8_t length;

		if (value > 15)
			value = 15;

		length = 1;

		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_SET_MIXER_GAIN,
			0,
			value,
			&retval,
			length,
			0);

		if (result < length)
		{
			return AIRSPY_ERROR_LIBUSB;
		}
		else
		{
			return AIRSPY_SUCCESS;
		}
	}

	int ADDCALL airspy_set_vga_gain(airspy_device_t *device, uint8_t value)
	{
		int result;
		uint8_t retval;
		uint8_t length;

		if (value > 15)
			value = 15;

		length = 1;

		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_SET_VGA_GAIN,
			0,
			value,
			&retval,
			length,
			0);

		if (result < length)
		{
			return AIRSPY_ERROR_LIBUSB;
		}
		else
		{
			return AIRSPY_SUCCESS;
		}
	}

	int ADDCALL airspy_set_lna_agc(airspy_device_t *device, uint8_t value)
	{
		int result;
		uint8_t retval;
		uint8_t length;

		length = 1;

		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_SET_LNA_AGC,
			0,
			value,
			&retval,
			length,
			0);

		if (result < length)
		{
			return AIRSPY_ERROR_LIBUSB;
		}
		else
		{
			return AIRSPY_SUCCESS;
		}
	}

	int ADDCALL airspy_set_mixer_agc(airspy_device_t *device, uint8_t value)
	{
		int result;
		uint8_t retval;
		uint8_t length;

		length = 1;

		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_SET_MIXER_AGC,
			0,
			value,
			&retval,
			length,
			0);

		if (result < length)
		{
			return AIRSPY_ERROR_LIBUSB;
		}
		else
		{
			return AIRSPY_SUCCESS;
		}
	}

	int ADDCALL airspy_set_linearity_gain(struct airspy_device *device, uint8_t value)
	{
		int rc;

		if (value >= GAIN_COUNT)
		{
			value = GAIN_COUNT - 1;
		}

		value = GAIN_COUNT - 1 - value;

		rc = airspy_set_mixer_agc(device, 0);
		if (rc < 0)
			return rc;

		rc = airspy_set_lna_agc(device, 0);
		if (rc < 0)
			return rc;

		rc = airspy_set_vga_gain(device, airspy_linearity_vga_gains[value]);
		if (rc < 0)
			return rc;

		rc = airspy_set_mixer_gain(device, airspy_linearity_mixer_gains[value]);
		if (rc < 0)
			return rc;

		rc = airspy_set_lna_gain(device, airspy_linearity_lna_gains[value]);
		if (rc < 0)
			return rc;

		return AIRSPY_SUCCESS;
	}

	int ADDCALL airspy_set_sensitivity_gain(struct airspy_device *device, uint8_t value)
	{
		int rc;

		if (value >= GAIN_COUNT)
		{
			value = GAIN_COUNT - 1;
		}

		value = GAIN_COUNT - 1 - value;

		rc = airspy_set_mixer_agc(device, 0);
		if (rc < 0)
			return rc;

		rc = airspy_set_lna_agc(device, 0);
		if (rc < 0)
			return rc;

		rc = airspy_set_vga_gain(device, airspy_sensitivity_vga_gains[value]);
		if (rc < 0)
			return rc;

		rc = airspy_set_mixer_gain(device, airspy_sensitivity_mixer_gains[value]);
		if (rc < 0)
			return rc;

		rc = airspy_set_lna_gain(device, airspy_sensitivity_lna_gains[value]);
		if (rc < 0)
			return rc;

		return AIRSPY_SUCCESS;
	}

	int ADDCALL airspy_set_rf_bias(airspy_device_t *device, uint8_t value)
	{
		return airspy_gpio_write(device, GPIO_PORT1, GPIO_PIN13, value);
	}

	int ADDCALL airspy_set_packing(airspy_device_t *device, uint8_t value)
	{
		int result;
		uint8_t retval;
		bool packing_enabled;

		if (device->streaming)
		{
			return AIRSPY_ERROR_BUSY;
		}

		result = libusb_control_transfer(
			device->usb_device,
			LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
			AIRSPY_SET_PACKING,
			0,
			value,
			&retval,
			1,
			0);

		if (result < 1)
		{
			return AIRSPY_ERROR_LIBUSB;
		}

		packing_enabled = value ? true : false;
		if (packing_enabled != device->packing_enabled)
		{
			cancel_transfers(device);
			free_transfers(device);

			device->packing_enabled = packing_enabled;
			device->buffer_size = packing_enabled ? (6144 * 24) : 262144;

			result = allocate_transfers(device);
			if (result != 0)
			{
				return AIRSPY_ERROR_NO_MEM;
			}
		}

		return AIRSPY_SUCCESS;
	}

	int ADDCALL airspy_is_streaming(airspy_device_t *device)
	{
		return (device->streaming == true && device->stop_requested == false);
	}

	const char *ADDCALL airspy_error_name(enum airspy_error errcode)
	{
		switch (errcode)
		{
		case AIRSPY_SUCCESS:
			return "AIRSPY_SUCCESS";

		case AIRSPY_TRUE:
			return "AIRSPY_TRUE";

		case AIRSPY_ERROR_INVALID_PARAM:
			return "AIRSPY_ERROR_INVALID_PARAM";

		case AIRSPY_ERROR_NOT_FOUND:
			return "AIRSPY_ERROR_NOT_FOUND";

		case AIRSPY_ERROR_BUSY:
			return "AIRSPY_ERROR_BUSY";

		case AIRSPY_ERROR_NO_MEM:
			return "AIRSPY_ERROR_NO_MEM";

		case AIRSPY_ERROR_LIBUSB:
			return "AIRSPY_ERROR_LIBUSB";

		case AIRSPY_ERROR_THREAD:
			return "AIRSPY_ERROR_THREAD";

		case AIRSPY_ERROR_STREAMING_THREAD_ERR:
			return "AIRSPY_ERROR_STREAMING_THREAD_ERR";

		case AIRSPY_ERROR_STREAMING_STOPPED:
			return "AIRSPY_ERROR_STREAMING_STOPPED";

		case AIRSPY_ERROR_OTHER:
			return "AIRSPY_ERROR_OTHER";

		default:
			return "airspy unknown error";
		}
	}

	const char *ADDCALL airspy_board_id_name(enum airspy_board_id board_id)
	{
		switch (board_id)
		{
		case AIRSPY_BOARD_ID_PROTO_AIRSPY:
			return "AIRSPY";

		case AIRSPY_BOARD_ID_INVALID:
			return "Invalid Board ID";

		default:
			return "Unknown Board ID";
		}
	}

#ifdef __cplusplus
} // __cplusplus defined.
#endif
