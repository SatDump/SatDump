/*
 * Copyright (C) 2013 by Miroslav Slugen <thunder.m@email.cz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* potřebné funkce */
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#if !defined (_WIN32) || defined(__MINGW32__)
#include <unistd.h>
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef __ANDROID__
#include <libusb-1.0/libusb.h>
#else
#include "libusb.h"
#endif

#ifndef LIBUSB_CALL
#define LIBUSB_CALL
#endif

/* hlavní hlavičkový soubor */
#include "mirisdr.h"

/* interní definice */
#include "constants.h"
#include "structs.h"

/* interní funkce - inline */
#include "reg.c"
#include "adc.c"
#include "convert/base.c"
#include "async.c"
#include "devices.c"
#include "gain.c"
#include "hard.c"
#include "streaming.c"
#include "soft.c"
#include "sync.c"

int mirisdr_open (mirisdr_dev_t **p, uint32_t index) {
    mirisdr_dev_t *dev = NULL;
    libusb_device **list, *device = NULL;
    struct libusb_device_descriptor dd;
    ssize_t i, i_max;
    size_t count = 0;
    int r;

    *p = NULL;

    if (!(dev = malloc(sizeof(*dev)))) return -ENOMEM;

    memset(dev, 0, sizeof(*dev));

    /* ostatní parametry */
    dev->index = index;

    libusb_init(&dev->ctx);
    i_max = libusb_get_device_list(dev->ctx, &list);

    for (i = 0; i < i_max; i++) {
        libusb_get_device_descriptor(list[i], &dd);

        if ((mirisdr_device_get(dd.idVendor, dd.idProduct)) &&
            (count++ == index)) {
            device = list[i];
            break;
        }
    }

    /* nenašli jsme zařízení */
    if (!device) {
        libusb_free_device_list(list, 1);
        fprintf( stderr, "no miri device %u found\n", dev->index);
        goto failed;
    }

    /* otevření zařízení */
    if ((r = libusb_open(device, &dev->dh)) < 0) {
        libusb_free_device_list(list, 1);
        fprintf( stderr, "failed to open miri usb device %u with code %d\n", dev->index, r);
        goto failed;
    }

    libusb_free_device_list(list, 1);

    /* reset je potřeba, jinak občas zařízení odmítá komunikovat */
    mirisdr_reset(dev);

    /* ještě je třeba vždy ukončit i streamování, které může být při otevření aktivní */
    mirisdr_streaming_stop(dev);
    mirisdr_adc_stop(dev);

    if (libusb_kernel_driver_active(dev->dh, 0) == 1) {
        dev->driver_active = 1;

#ifdef DETACH_KERNEL_DRIVER
        if (!libusb_detach_kernel_driver(dev->dh, 0)) {
            fprintf(stderr, "Detached kernel driver\n");
        } else {
            fprintf(stderr, "Detaching kernel driver failed!");
            dev->driver_active = 0;
            goto failed;
        }
#else
        fprintf(stderr, "\nKernel driver is active, or device is "
                "claimed by second instance of libmirisdr."
                "\nIn the first case, please either detach"
                " or blacklist the kernel module\n"
                "(msi001 and msi2500), or enable automatic"
                " detaching at compile time.\n\n");
#endif
    } else {
        dev->driver_active = 0;
    }

    if ((r = libusb_claim_interface(dev->dh, 0)) < 0) {
        fprintf( stderr, "failed to claim miri usb device %u with code %d\n", dev->index, r);
        goto failed;
    }

    /* inicializace tuneru */
    dev->freq = DEFAULT_FREQ;
    dev->rate = DEFAULT_RATE;
    dev->gain = DEFAULT_GAIN;
    dev->band = MIRISDR_BAND_VHF; // matches always the default frequency of 90 MHz

    dev->gain_reduction_lna = 0;
    dev->gain_reduction_mixer = 0;
    dev->gain_reduction_baseband = 43;
    dev->if_freq = MIRISDR_IF_ZERO;
    dev->format_auto = MIRISDR_FORMAT_AUTO_ON;
    dev->bandwidth = MIRISDR_BW_8MHZ;
    dev->xtal = MIRISDR_XTAL_24M;
    dev->bias = 0;

    dev->hw_flavour = MIRISDR_HW_DEFAULT;

    /* ISOC is more stable but works only on Unix systems */
#if !defined (_WIN32) || defined(__MINGW32__)
    dev->transfer = MIRISDR_TRANSFER_ISOC;
#else
    dev->transfer = MIRISDR_TRANSFER_BULK;
    /* SatDump patch. TODO: Once upstreamed in LibUSB, put version guards here */
//#if LIBUSB_API_VERSION >= 0x0100010B
	if(libusb_endpoint_supports_raw_io(dev->dh, 0x81) == 1) {
		libusb_endpoint_set_raw_io(dev->dh, 0x81, 1);
	}
//#endif
#endif

    mirisdr_adc_init(dev);
    mirisdr_set_hard(dev);
    mirisdr_set_soft(dev);
    mirisdr_set_gain(dev);

    *p = dev;

    return 0;

failed:
    if (dev) {
        if (dev->dh) {
            libusb_release_interface(dev->dh, 0);
            libusb_close(dev->dh);
        }
        if (dev->ctx) libusb_exit(dev->ctx);
        free(dev);
    }

    return -1;
}

#ifdef __ANDROID__
int mirisdr_open_fd (mirisdr_dev_t **p, uint32_t index, int fd) {
    mirisdr_dev_t *dev = NULL;
    // libusb_device **list, *device = NULL;
    struct libusb_device_descriptor dd;
    ssize_t i, i_max;
    size_t count = 0;
    int r;

    *p = NULL;

    if (!(dev = malloc(sizeof(*dev)))) return -ENOMEM;

    memset(dev, 0, sizeof(*dev));

    /* ostatní parametry */
    dev->index = index;

    libusb_set_option(NULL, LIBUSB_OPTION_NO_DEVICE_DISCOVERY, NULL);
    libusb_init(&dev->ctx);

    libusb_wrap_sys_device(dev->ctx, (intptr_t)fd, &dev->dh);

    /* reset je potřeba, jinak občas zařízení odmítá komunikovat */
    mirisdr_reset(dev);

    /* ještě je třeba vždy ukončit i streamování, které může být při otevření aktivní */
    mirisdr_streaming_stop(dev);
    mirisdr_adc_stop(dev);

    if (libusb_kernel_driver_active(dev->dh, 0) == 1) {
        dev->driver_active = 1;

#ifdef DETACH_KERNEL_DRIVER
        if (!libusb_detach_kernel_driver(dev->dh, 0)) {
            fprintf(stderr, "Detached kernel driver\n");
        } else {
            fprintf(stderr, "Detaching kernel driver failed!");
            dev->driver_active = 0;
            goto failed;
        }
#else
        fprintf(stderr, "\nKernel driver is active, or device is "
                "claimed by second instance of libmirisdr."
                "\nIn the first case, please either detach"
                " or blacklist the kernel module\n"
                "(msi001 and msi2500), or enable automatic"
                " detaching at compile time.\n\n");
#endif
    } else {
        dev->driver_active = 0;
    }

    if ((r = libusb_claim_interface(dev->dh, 0)) < 0) {
        fprintf( stderr, "failed to claim miri usb device %u with code %d\n", dev->index, r);
        goto failed;
    }

    /* inicializace tuneru */
    dev->freq = DEFAULT_FREQ;
    dev->rate = DEFAULT_RATE;
    dev->gain = DEFAULT_GAIN;
    dev->band = MIRISDR_BAND_VHF; // matches always the default frequency of 90 MHz

    dev->gain_reduction_lna = 0;
    dev->gain_reduction_mixer = 0;
    dev->gain_reduction_baseband = 43;
    dev->if_freq = MIRISDR_IF_ZERO;
    dev->format_auto = MIRISDR_FORMAT_AUTO_ON;
    dev->bandwidth = MIRISDR_BW_8MHZ;
    dev->xtal = MIRISDR_XTAL_24M;
    dev->bias = 0;

    dev->hw_flavour = MIRISDR_HW_DEFAULT;

    /* ISOC is more stable but works only on Unix systems */
#if !defined (_WIN32) || defined(__MINGW32__)
    dev->transfer = MIRISDR_TRANSFER_ISOC;
#else
    dev->transfer = MIRISDR_TRANSFER_BULK;
#endif

    mirisdr_adc_init(dev);
    mirisdr_set_hard(dev);
    mirisdr_set_soft(dev);
    mirisdr_set_gain(dev);

    *p = dev;

    return 0;

failed:
    if (dev) {
        if (dev->dh) {
            libusb_release_interface(dev->dh, 0);
            libusb_close(dev->dh);
        }
        if (dev->ctx) libusb_exit(dev->ctx);
        free(dev);
    }

    return -1;
}
#endif

int mirisdr_close (mirisdr_dev_t *p) {
    if (!p) goto failed;

    /* ukončení async čtení okamžitě */
    mirisdr_cancel_async_now(p);

    // similar to rtl-sdr
#if defined(_WIN32) && !defined(__MINGW32__)
            Sleep(1);
#else
            usleep(1000);
#endif

    /* deinicializace tuneru */
    if (p->dh)
    {
        libusb_release_interface(p->dh, 0);

#ifdef DETACH_KERNEL_DRIVER
        if (p->driver_active) {
            if (!libusb_attach_kernel_driver(p->dh, 0))
                fprintf(stderr, "Reattached kernel driver\n");
            else
                fprintf(stderr, "Reattaching kernel driver failed!\n");
        }
#endif
        if (p->async_status != MIRISDR_ASYNC_FAILED) {
            libusb_close(p->dh);
        }
    }

    if (p->ctx) libusb_exit(p->ctx);

    free(p);

    return 0;

failed:
    return -1;
}

int mirisdr_reset (mirisdr_dev_t *p) {
    int r;

    if (!p) goto failed;
    if (!p->dh) goto failed;

    /* měli bychom uvolnit zařízení předem? */

    if ((r = libusb_reset_device(p->dh)) < 0) {
        fprintf( stderr, "failed to reset miri usb device %u with code %d\n", p->index, r);
        goto failed;
    }

    return 0;

failed:
    return -1;
}

int mirisdr_reset_buffer (mirisdr_dev_t *p) {
    if (!p) goto failed;
    if (!p->dh) goto failed;

    /* zatím není jasné k čemu by bylo, proto pouze provedeme reset async části */
    mirisdr_stop_async(p);
    mirisdr_start_async(p);

    return 0;

failed:
    return -1;
}

int mirisdr_get_usb_strings (mirisdr_dev_t *dev, char *manufact, char *product, char *serial) {
(void) dev;
    fprintf( stderr, "mirisdr_get_usb_strings not implemented yet\n");

    memset(manufact, 0, 256);
    memset(product, 0, 256);
    memset(serial, 0, 256);

    return 0;
}

int mirisdr_set_hw_flavour (mirisdr_dev_t *p, mirisdr_hw_flavour_t hw_flavour) {
    if (!p) goto failed;

    p->hw_flavour = hw_flavour;
    return 0;

failed:
    return -1;
}