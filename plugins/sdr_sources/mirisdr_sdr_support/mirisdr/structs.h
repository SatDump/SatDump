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

typedef struct mirisdr_device {
    uint16_t            vid;
    uint16_t            pid;
    const char          *name;
    const char          *manufacturer;
    const char          *product;
} mirisdr_device_t;

struct mirisdr_dev {
    libusb_context      *ctx;
    struct libusb_device_handle *dh;

    /* parametry */
    uint32_t            index;
    uint32_t            freq;
    uint32_t            rate;
    int                 gain;
    int                 gain_reduction_lna;
    int                 gain_reduction_mixbuffer;
    int                 gain_reduction_mixer;
    int                 gain_reduction_baseband;
    mirisdr_hw_flavour_t hw_flavour;
    mirisdr_band_t      band;
    enum {
        MIRISDR_FORMAT_AUTO_ON = 0,
        MIRISDR_FORMAT_AUTO_OFF
    } format_auto;
    enum {
        MIRISDR_FORMAT_252_S16 = 0,
        MIRISDR_FORMAT_336_S16,
        MIRISDR_FORMAT_384_S16,
        MIRISDR_FORMAT_504_S16,
        MIRISDR_FORMAT_504_S8
    } format;
    enum {
        MIRISDR_BW_200KHZ = 0,
        MIRISDR_BW_300KHZ,
        MIRISDR_BW_600KHZ,
        MIRISDR_BW_1536KHZ,
        MIRISDR_BW_5MHZ,
        MIRISDR_BW_6MHZ,
        MIRISDR_BW_7MHZ,
        MIRISDR_BW_8MHZ
    } bandwidth;
    enum {
        MIRISDR_IF_ZERO = 0,
        MIRISDR_IF_450KHZ,
        MIRISDR_IF_1620KHZ,
        MIRISDR_IF_2048KHZ
    } if_freq;
    enum {
        MIRISDR_XTAL_19_2M = 0,
        MIRISDR_XTAL_22M,
        MIRISDR_XTAL_24M,
        MIRISDR_XTAL_24_576M,
        MIRISDR_XTAL_26M,
        MIRISDR_XTAL_38_4M
    } xtal;
    enum {
        MIRISDR_TRANSFER_BULK = 0,
        MIRISDR_TRANSFER_ISOC
    } transfer;

    /* async */
    enum {
        MIRISDR_ASYNC_INACTIVE = 0,
        MIRISDR_ASYNC_CANCELING,
        MIRISDR_ASYNC_RUNNING,
        MIRISDR_ASYNC_PAUSED,
        MIRISDR_ASYNC_FAILED
    } async_status;
    mirisdr_read_async_cb_t cb;
    void                *cb_ctx;
    size_t              xfer_buf_num;
    struct libusb_transfer **xfer;
    unsigned char       **xfer_buf;
    size_t              xfer_out_len;
    size_t              xfer_out_pos;
    unsigned char       *xfer_out;
    uint32_t            addr;
    int                 driver_active;
    int                 bias;
    int                 reg8;
};

