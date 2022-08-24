/*
 * Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de>
 * Copyright (C) 2012 by Dimitri Stolnikov <horiz0n@gmx.net>
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

#ifndef __MIRISDR_H
#define __MIRISDR_H

#ifdef __cplusplus
extern "C" {
#endif

// Set debug level
// 0=no debug
// 1=gain and frequency info.
// 2=extended debug
#define MIRISDR_DEBUG 0

#include <stdint.h>
#include "mirisdr_export.h"

typedef enum
{
    MIRISDR_HW_DEFAULT,
    MIRISDR_HW_SDRPLAY,
} mirisdr_hw_flavour_t;

typedef enum
{
    MIRISDR_BAND_AM1,
    MIRISDR_BAND_AM2,
    MIRISDR_BAND_VHF,
    MIRISDR_BAND_3,
    MIRISDR_BAND_45,
    MIRISDR_BAND_L,
} mirisdr_band_t;

typedef struct mirisdr_dev mirisdr_dev_t;

/* devices */
MIRISDR_API uint32_t mirisdr_get_device_count (void);
MIRISDR_API const char *mirisdr_get_device_name (uint32_t index);
MIRISDR_API int mirisdr_get_device_usb_strings (uint32_t index, char *manufact, char *product, char *serial);

/* main */
MIRISDR_API int mirisdr_open (mirisdr_dev_t **p, uint32_t index);
#ifdef __ANDROID__
MIRISDR_API int mirisdr_open_fd (mirisdr_dev_t **p, uint32_t index, int fd);
#endif
MIRISDR_API int mirisdr_close (mirisdr_dev_t *p);
MIRISDR_API int mirisdr_reset (mirisdr_dev_t *p);                       /* extra */
MIRISDR_API int mirisdr_reset_buffer (mirisdr_dev_t *p);
MIRISDR_API int mirisdr_get_usb_strings (mirisdr_dev_t *dev, char *manufact, char *product, char *serial);
MIRISDR_API int mirisdr_set_hw_flavour (mirisdr_dev_t *p, mirisdr_hw_flavour_t hw_flavour);

/* sync */
MIRISDR_API int mirisdr_read_sync (mirisdr_dev_t *p, void *buf, int len, int *n_read);

/* async */
typedef void(*mirisdr_read_async_cb_t) (unsigned char *buf, uint32_t len, void *ctx);
MIRISDR_API int mirisdr_read_async (mirisdr_dev_t *p, mirisdr_read_async_cb_t cb, void *ctx, uint32_t num, uint32_t len);
MIRISDR_API int mirisdr_cancel_async (mirisdr_dev_t *p);
MIRISDR_API int mirisdr_cancel_async_now (mirisdr_dev_t *p);            /* extra */
MIRISDR_API int mirisdr_start_async (mirisdr_dev_t *p);                 /* extra */
MIRISDR_API int mirisdr_stop_async (mirisdr_dev_t *p);                  /* extra */

/* adc */
MIRISDR_API int mirisdr_adc_init (mirisdr_dev_t *p);                    /* extra */

/* rate control */
MIRISDR_API int mirisdr_set_sample_rate (mirisdr_dev_t *p, uint32_t rate);
MIRISDR_API uint32_t mirisdr_get_sample_rate (mirisdr_dev_t *p);

/* sample format control */
MIRISDR_API int mirisdr_set_sample_format (mirisdr_dev_t *p, char *v);  /* extra */
MIRISDR_API const char *mirisdr_get_sample_format (mirisdr_dev_t *p);   /* extra */

/* streaming control */
MIRISDR_API int mirisdr_streaming_start (mirisdr_dev_t *p);             /* extra */
MIRISDR_API int mirisdr_streaming_stop (mirisdr_dev_t *p);              /* extra */

/* frequency */
MIRISDR_API int mirisdr_set_center_freq (mirisdr_dev_t *p, uint32_t freq);
MIRISDR_API uint32_t mirisdr_get_center_freq (mirisdr_dev_t *p);
MIRISDR_API int mirisdr_set_if_freq (mirisdr_dev_t *p, uint32_t freq);  /* extra */
MIRISDR_API uint32_t mirisdr_get_if_freq (mirisdr_dev_t *p);            /* extra */
MIRISDR_API int mirisdr_set_xtal_freq (mirisdr_dev_t *p, uint32_t freq);/* extra */
MIRISDR_API uint32_t mirisdr_get_xtal_freq (mirisdr_dev_t *p);          /* extra */
MIRISDR_API int mirisdr_set_bandwidth (mirisdr_dev_t *p, uint32_t bw);  /* extra */
MIRISDR_API uint32_t mirisdr_get_bandwidth (mirisdr_dev_t *p);          /* extra */
MIRISDR_API int mirisdr_set_offset_tuning (mirisdr_dev_t *p, int on);   /* extra */
MIRISDR_API mirisdr_band_t mirisdr_get_band (mirisdr_dev_t *p);         /* extra */

/* not implemented yet */
MIRISDR_API int mirisdr_set_freq_correction (mirisdr_dev_t *p, int ppm);
MIRISDR_API int mirisdr_set_direct_sampling (mirisdr_dev_t *p, int on);

/* transfer */
MIRISDR_API int mirisdr_set_transfer (mirisdr_dev_t *p, char *v);       /* extra */
MIRISDR_API const char *mirisdr_get_transfer (mirisdr_dev_t *p);        /* extra */

/* gain */
MIRISDR_API int mirisdr_set_gain (mirisdr_dev_t *p);                    /* extra */
MIRISDR_API int mirisdr_get_tuner_gains (mirisdr_dev_t *dev, int *gains);
MIRISDR_API int mirisdr_set_tuner_gain (mirisdr_dev_t *p, int gain);
MIRISDR_API int mirisdr_get_tuner_gain (mirisdr_dev_t *p);              /* extra */
MIRISDR_API int mirisdr_set_tuner_gain_mode (mirisdr_dev_t *p, int mode);
MIRISDR_API int mirisdr_get_tuner_gain_mode (mirisdr_dev_t *p);         /* extra */
MIRISDR_API int mirisdr_set_mixer_gain (mirisdr_dev_t *p, int gain);    /* extra */
MIRISDR_API int mirisdr_set_mixbuffer_gain (mirisdr_dev_t *p, int gain);/* extra */
MIRISDR_API int mirisdr_set_lna_gain (mirisdr_dev_t *p, int gain);      /* extra */
MIRISDR_API int mirisdr_set_baseband_gain (mirisdr_dev_t *p, int gain); /* extra */
MIRISDR_API int mirisdr_get_mixer_gain (mirisdr_dev_t *p);              /* extra */
MIRISDR_API int mirisdr_get_mixbuffer_gain (mirisdr_dev_t *p);          /* extra */
MIRISDR_API int mirisdr_get_lna_gain (mirisdr_dev_t *p);                /* extra */
MIRISDR_API int mirisdr_get_baseband_gain (mirisdr_dev_t *p);           /* extra */
MIRISDR_API int mirisdr_set_bias (mirisdr_dev_t *p, int bias);          /* extra */
MIRISDR_API int mirisdr_get_bias (mirisdr_dev_t *p);                    /* extra */

#ifdef __cplusplus
}
#endif

#endif /* __MIRISDR_H */
