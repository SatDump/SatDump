/*
 * Rafael Micro R820T/R828D driver
 *
 * Copyright (C) 2013 Mauro Carvalho Chehab <mchehab@redhat.com>
 * Copyright (C) 2013 Steve Markgraf <steve@steve-m.de>
 *
 * This driver is a heavily modified version of the driver found in the
 * Linux kernel:
 * http://git.linuxtv.org/linux-2.6.git/history/HEAD:/drivers/media/tuners/r820t.c
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "rtlsdr_i2c.h"
#include "tuner_r82xx.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MHZ(x)		((x)*1000*1000)
#define KHZ(x)		((x)*1000)

/*
 * Static constants
 */

/* Those initial values start from REG_SHADOW_START */
static const uint8_t r82xx_init_array[NUM_REGS] = {
	0x83, 0x32, 0x75,			/* 05 to 07 */
	0xc0, 0x40, 0xd6, 0x6c,			/* 08 to 0b */
	0xf5, 0x63, 0x75, 0x68,			/* 0c to 0f */
	0x6c, 0x83, 0x80, 0x00,			/* 10 to 13 */
	0x0f, 0x00, 0xc0, 0x30,			/* 14 to 17 */
	0x48, 0xcc, 0x60, 0x00,			/* 18 to 1b */
	0x54, 0xae, 0x4a, 0xc0			/* 1c to 1f */
};

/* Tuner frequency ranges */
static const struct r82xx_freq_range freq_ranges[] = {
	{
	/* .freq = */		0,	/* Start freq, in MHz */
	/* .open_d = */		0x08,	/* low */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */		0xdf,	/* R27[7:0]  band2,band0 */
	/* .xtal_cap20p = */	0x02,	/* R16[1:0]  20pF (10)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */	0x00,
	}, {
	/* .freq = */		50,	/* Start freq, in MHz */
	/* .open_d = */		0x08,	/* low */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */		0xbe,	/* R27[7:0]  band4,band1  */
	/* .xtal_cap20p = */	0x02,	/* R16[1:0]  20pF (10)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */	0x00,
	}, {
	/* .freq = */		55,	/* Start freq, in MHz */
	/* .open_d = */		0x08,	/* low */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */		0x8b,	/* R27[7:0]  band7,band4 */
	/* .xtal_cap20p = */	0x02,	/* R16[1:0]  20pF (10)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */	0x00,
	}, {
	/* .freq = */		60,	/* Start freq, in MHz */
	/* .open_d = */		0x08,	/* low */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */		0x7b,	/* R27[7:0]  band8,band4 */
	/* .xtal_cap20p = */	0x02,	/* R16[1:0]  20pF (10)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */	0x00,
	}, {
	/* .freq = */		65,	/* Start freq, in MHz */
	/* .open_d = */		0x08,	/* low */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */		0x69,	/* R27[7:0]  band9,band6 */
	/* .xtal_cap20p = */	0x02,	/* R16[1:0]  20pF (10)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */	0x00,
	}, {
	/* .freq = */		70,	/* Start freq, in MHz */
	/* .open_d = */		0x08,	/* low */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */		0x58,	/* R27[7:0]  band10,band7 */
	/* .xtal_cap20p = */	0x02,	/* R16[1:0]  20pF (10)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */	0x00,
	}, {
	/* .freq = */		75,	/* Start freq, in MHz */
	/* .open_d = */		0x00,	/* high */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */		0x44,	/* R27[7:0]  band11,band11 */
	/* .xtal_cap20p = */	0x02,	/* R16[1:0]  20pF (10)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */	0x00,
	}, {
	/* .freq = */		80,	/* Start freq, in MHz */
	/* .open_d = */		0x00,	/* high */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */		0x44,	/* R27[7:0]  band11,band11 */
	/* .xtal_cap20p = */	0x02,	/* R16[1:0]  20pF (10)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */	0x00,
	}, {
	/* .freq = */		90,	/* Start freq, in MHz */
	/* .open_d = */		0x00,	/* high */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */		0x34,	/* R27[7:0]  band12,band11 */
	/* .xtal_cap20p = */	0x01,	/* R16[1:0]  10pF (01)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */	0x00,
	}, {
	/* .freq = */		100,	/* Start freq, in MHz */
	/* .open_d = */		0x00,	/* high */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */		0x34,	/* R27[7:0]  band12,band11 */
	/* .xtal_cap20p = */	0x01,	/* R16[1:0]  10pF (01)    */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */	0x00,
	}, {
	/* .freq = */		110,	/* Start freq, in MHz */
	/* .open_d = */		0x00,	/* high */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */		0x24,	/* R27[7:0]  band13,band11 */
	/* .xtal_cap20p = */	0x01,	/* R16[1:0]  10pF (01)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */	0x00,
	}, {
	/* .freq = */		120,	/* Start freq, in MHz */
	/* .open_d = */		0x00,	/* high */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */		0x24,	/* R27[7:0]  band13,band11 */
	/* .xtal_cap20p = */	0x01,	/* R16[1:0]  10pF (01)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */	0x00,
	}, {
	/* .freq = */		140,	/* Start freq, in MHz */
	/* .open_d = */		0x00,	/* high */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */		0x14,	/* R27[7:0]  band14,band11 */
	/* .xtal_cap20p = */	0x01,	/* R16[1:0]  10pF (01)   */
	/* .xtal_cap10p = */	0x01,
	/* .xtal_cap0p = */	0x00,
	}, {
	/* .freq = */		180,	/* Start freq, in MHz */
	/* .open_d = */		0x00,	/* high */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */		0x13,	/* R27[7:0]  band14,band12 */
	/* .xtal_cap20p = */	0x00,	/* R16[1:0]  0pF (00)   */
	/* .xtal_cap10p = */	0x00,
	/* .xtal_cap0p = */	0x00,
	}, {
	/* .freq = */		220,	/* Start freq, in MHz */
	/* .open_d = */		0x00,	/* high */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */		0x13,	/* R27[7:0]  band14,band12 */
	/* .xtal_cap20p = */	0x00,	/* R16[1:0]  0pF (00)   */
	/* .xtal_cap10p = */	0x00,
	/* .xtal_cap0p = */	0x00,
	}, {
	/* .freq = */		250,	/* Start freq, in MHz */
	/* .open_d = */		0x00,	/* high */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */		0x11,	/* R27[7:0]  highest,highest */
	/* .xtal_cap20p = */	0x00,	/* R16[1:0]  0pF (00)   */
	/* .xtal_cap10p = */	0x00,
	/* .xtal_cap0p = */	0x00,
	}, {
	/* .freq = */		280,	/* Start freq, in MHz */
	/* .open_d = */		0x00,	/* high */
	/* .rf_mux_ploy = */	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
	/* .tf_c = */		0x00,	/* R27[7:0]  highest,highest */
	/* .xtal_cap20p = */	0x00,	/* R16[1:0]  0pF (00)   */
	/* .xtal_cap10p = */	0x00,
	/* .xtal_cap0p = */	0x00,
	}, {
	/* .freq = */		310,	/* Start freq, in MHz */
	/* .open_d = */		0x00,	/* high */
	/* .rf_mux_ploy = */	0x41,	/* R26[7:6]=1 (bypass)  R26[1:0]=1 (middle) */
	/* .tf_c = */		0x00,	/* R27[7:0]  highest,highest */
	/* .xtal_cap20p = */	0x00,	/* R16[1:0]  0pF (00)   */
	/* .xtal_cap10p = */	0x00,
	/* .xtal_cap0p = */	0x00,
	}, {
	/* .freq = */		450,	/* Start freq, in MHz */
	/* .open_d = */		0x00,	/* high */
	/* .rf_mux_ploy = */	0x41,	/* R26[7:6]=1 (bypass)  R26[1:0]=1 (middle) */
	/* .tf_c = */		0x00,	/* R27[7:0]  highest,highest */
	/* .xtal_cap20p = */	0x00,	/* R16[1:0]  0pF (00)   */
	/* .xtal_cap10p = */	0x00,
	/* .xtal_cap0p = */	0x00,
	}, {
	/* .freq = */		588,	/* Start freq, in MHz */
	/* .open_d = */		0x00,	/* high */
	/* .rf_mux_ploy = */	0x40,	/* R26[7:6]=1 (bypass)  R26[1:0]=0 (highest) */
	/* .tf_c = */		0x00,	/* R27[7:0]  highest,highest */
	/* .xtal_cap20p = */	0x00,	/* R16[1:0]  0pF (00)   */
	/* .xtal_cap10p = */	0x00,
	/* .xtal_cap0p = */	0x00,
	}, {
	/* .freq = */		650,	/* Start freq, in MHz */
	/* .open_d = */		0x00,	/* high */
	/* .rf_mux_ploy = */	0x40,	/* R26[7:6]=1 (bypass)  R26[1:0]=0 (highest) */
	/* .tf_c = */		0x00,	/* R27[7:0]  highest,highest */
	/* .xtal_cap20p = */	0x00,	/* R16[1:0]  0pF (00)   */
	/* .xtal_cap10p = */	0x00,
	/* .xtal_cap0p = */	0x00,
	}
};

static int r82xx_xtal_capacitor[][2] = {
	{ 0x0b, XTAL_LOW_CAP_30P },
	{ 0x02, XTAL_LOW_CAP_20P },
	{ 0x01, XTAL_LOW_CAP_10P },
	{ 0x00, XTAL_LOW_CAP_0P  },
	{ 0x10, XTAL_HIGH_CAP_0P },
};

/*
 * I2C read/write code and shadow registers logic
 */
static void shadow_store(struct r82xx_priv *priv, uint8_t reg, const uint8_t *val,
			 int len)
{
	int r = reg - REG_SHADOW_START;

	if (r < 0) {
		len += r;
		r = 0;
	}
	if (len <= 0)
		return;
	if (len > NUM_REGS - r)
		len = NUM_REGS - r;

	memcpy(&priv->regs[r], val, len);
}

static int r82xx_write(struct r82xx_priv *priv, uint8_t reg, const uint8_t *val,
		       unsigned int len)
{
	int rc, size, pos = 0;

	/* Store the shadow registers */
	shadow_store(priv, reg, val, len);

	do {
		if (len > priv->cfg->max_i2c_msg_len - 1)
			size = priv->cfg->max_i2c_msg_len - 1;
		else
			size = len;

		/* Fill I2C buffer */
		priv->buf[0] = reg;
		memcpy(&priv->buf[1], &val[pos], size);

		rc = rtlsdr_i2c_write_fn(priv->rtl_dev, priv->cfg->i2c_addr,
					 priv->buf, size + 1);

		if (rc != size + 1) {
			fprintf(stderr, "%s: i2c wr failed=%d reg=%02x len=%d\n",
				   __FUNCTION__, rc, reg, size);
			if (rc < 0)
				return rc;
			return -1;
		}

		reg += size;
		len -= size;
		pos += size;
	} while (len > 0);

	return 0;
}

static int r82xx_write_reg(struct r82xx_priv *priv, uint8_t reg, uint8_t val)
{
	return r82xx_write(priv, reg, &val, 1);
}

static int r82xx_read_cache_reg(struct r82xx_priv *priv, int reg)
{
	reg -= REG_SHADOW_START;

	if (reg >= 0 && reg < NUM_REGS)
		return priv->regs[reg];
	else
		return -1;
}

static int r82xx_write_reg_mask(struct r82xx_priv *priv, uint8_t reg, uint8_t val,
				uint8_t bit_mask)
{
	int rc = r82xx_read_cache_reg(priv, reg);

	if (rc < 0)
		return rc;

	val = (rc & ~bit_mask) | (val & bit_mask);

	return r82xx_write(priv, reg, &val, 1);
}

static uint8_t r82xx_bitrev(uint8_t byte)
{
	const uint8_t lut[16] = { 0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
				  0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf };

	return (lut[byte & 0xf] << 4) | lut[byte >> 4];
}

static int r82xx_read(struct r82xx_priv *priv, uint8_t reg, uint8_t *val, int len)
{
	int rc, i;
	uint8_t *p = &priv->buf[1];

	priv->buf[0] = reg;

	rc = rtlsdr_i2c_write_fn(priv->rtl_dev, priv->cfg->i2c_addr, priv->buf, 1);
	if (rc < 1)
		return rc;

	rc = rtlsdr_i2c_read_fn(priv->rtl_dev, priv->cfg->i2c_addr, p, len);

	if (rc != len) {
		fprintf(stderr, "%s: i2c rd failed=%d reg=%02x len=%d\n",
			   __FUNCTION__, rc, reg, len);
		if (rc < 0)
			return rc;
		return -1;
	}

	/* Copy data to the output buffer */
	for (i = 0; i < len; i++)
		val[i] = r82xx_bitrev(p[i]);

	return 0;
}

/*
 * r82xx tuning logic
 */

static int r82xx_set_mux(struct r82xx_priv *priv, uint32_t freq)
{
	const struct r82xx_freq_range *range;
	int rc;
	unsigned int i;
	uint8_t val;

	/* Get the proper frequency range */
	freq = freq / 1000000;
	for (i = 0; i < ARRAY_SIZE(freq_ranges) - 1; i++) {
		if (freq < freq_ranges[i + 1].freq)
			break;
	}
	range = &freq_ranges[i];

	/* Open Drain */
	rc = r82xx_write_reg_mask(priv, 0x17, range->open_d, 0x08);
	if (rc < 0)
		return rc;

	/* RF_MUX,Polymux */
	rc = r82xx_write_reg_mask(priv, 0x1a, range->rf_mux_ploy, 0xc3);
	if (rc < 0)
		return rc;

	/* TF BAND */
	rc = r82xx_write_reg(priv, 0x1b, range->tf_c);
	if (rc < 0)
		return rc;

	/* XTAL CAP & Drive */
	switch (priv->xtal_cap_sel) {
	case XTAL_LOW_CAP_30P:
	case XTAL_LOW_CAP_20P:
		val = range->xtal_cap20p | 0x08;
		break;
	case XTAL_LOW_CAP_10P:
		val = range->xtal_cap10p | 0x08;
		break;
	case XTAL_HIGH_CAP_0P:
		val = range->xtal_cap0p | 0x00;
		break;
	default:
	case XTAL_LOW_CAP_0P:
		val = range->xtal_cap0p | 0x08;
		break;
	}
	rc = r82xx_write_reg_mask(priv, 0x10, val, 0x0b);
	if (rc < 0)
		return rc;

	rc = r82xx_write_reg_mask(priv, 0x08, 0x00, 0x3f);
	if (rc < 0)
		return rc;

	rc = r82xx_write_reg_mask(priv, 0x09, 0x00, 0x3f);

	return rc;
}

static int r82xx_set_pll(struct r82xx_priv *priv, uint32_t freq)
{
	int rc, i;
	unsigned sleep_time = 10000;
	uint64_t vco_freq;
	uint32_t vco_fra;	/* VCO contribution by SDM (kHz) */
	uint32_t vco_min = 1770000;
	uint32_t vco_max = vco_min * 2;
	uint32_t freq_khz, pll_ref, pll_ref_khz;
	uint16_t n_sdm = 2;
	uint16_t sdm = 0;
	uint8_t mix_div = 2;
	uint8_t div_buf = 0;
	uint8_t div_num = 0;
	uint8_t vco_power_ref = 2;
	uint8_t refdiv2 = 0;
	uint8_t ni, si, nint, vco_fine_tune, val;
	uint8_t data[5];

	/* Frequency in kHz */
	freq_khz = (freq + 500) / 1000;
	pll_ref = priv->cfg->xtal;
	pll_ref_khz = (priv->cfg->xtal + 500) / 1000;

	rc = r82xx_write_reg_mask(priv, 0x10, refdiv2, 0x10);
	if (rc < 0)
		return rc;

	/* set pll autotune = 128kHz */
	rc = r82xx_write_reg_mask(priv, 0x1a, 0x00, 0x0c);
	if (rc < 0)
		return rc;

	/* set VCO current = 100 */
	rc = r82xx_write_reg_mask(priv, 0x12, 0x80, 0xe0);
	if (rc < 0)
		return rc;

	/* Calculate divider */
	while (mix_div <= 64) {
		if (((freq_khz * mix_div) >= vco_min) &&
		   ((freq_khz * mix_div) < vco_max)) {
			div_buf = mix_div;
			while (div_buf > 2) {
				div_buf = div_buf >> 1;
				div_num++;
			}
			break;
		}
		mix_div = mix_div << 1;
	}

	rc = r82xx_read(priv, 0x00, data, sizeof(data));
	if (rc < 0)
		return rc;

	if (priv->cfg->rafael_chip == CHIP_R828D)
		vco_power_ref = 1;

	vco_fine_tune = (data[4] & 0x30) >> 4;

	if (vco_fine_tune > vco_power_ref)
		div_num = div_num - 1;
	else if (vco_fine_tune < vco_power_ref)
		div_num = div_num + 1;

	rc = r82xx_write_reg_mask(priv, 0x10, div_num << 5, 0xe0);
	if (rc < 0)
		return rc;

	vco_freq = (uint64_t)freq * (uint64_t)mix_div;
	nint = vco_freq / (2 * pll_ref);
	vco_fra = (vco_freq - 2 * pll_ref * nint) / 1000;

	if (nint > ((128 / vco_power_ref) - 1)) {
		fprintf(stderr, "[R82XX] No valid PLL values for %u Hz!\n", freq);
		return -1;
	}

	ni = (nint - 13) / 4;
	si = nint - 4 * ni - 13;

	rc = r82xx_write_reg(priv, 0x14, ni + (si << 6));
	if (rc < 0)
		return rc;

	/* pw_sdm */
	if (!vco_fra)
		val = 0x08;
	else
		val = 0x00;

	rc = r82xx_write_reg_mask(priv, 0x12, val, 0x08);
	if (rc < 0)
		return rc;

	/* sdm calculator */
	while (vco_fra > 1) {
		if (vco_fra > (2 * pll_ref_khz / n_sdm)) {
			sdm = sdm + 32768 / (n_sdm / 2);
			vco_fra = vco_fra - 2 * pll_ref_khz / n_sdm;
			if (n_sdm >= 0x8000)
				break;
		}
		n_sdm <<= 1;
	}

	rc = r82xx_write_reg(priv, 0x16, sdm >> 8);
	if (rc < 0)
		return rc;
	rc = r82xx_write_reg(priv, 0x15, sdm & 0xff);
	if (rc < 0)
		return rc;

	for (i = 0; i < 2; i++) {
//		usleep_range(sleep_time, sleep_time + 1000);

		/* Check if PLL has locked */
		rc = r82xx_read(priv, 0x00, data, 3);
		if (rc < 0)
			return rc;
		if (data[2] & 0x40)
			break;

		if (!i) {
			/* Didn't lock. Increase VCO current */
			rc = r82xx_write_reg_mask(priv, 0x12, 0x60, 0xe0);
			if (rc < 0)
				return rc;
		}
	}

	if (!(data[2] & 0x40)) {
		fprintf(stderr, "[R82XX] PLL not locked!\n");
		priv->has_lock = 0;
		return 0;
	}

	priv->has_lock = 1;

	/* set pll autotune = 8kHz */
	rc = r82xx_write_reg_mask(priv, 0x1a, 0x08, 0x08);

	return rc;
}

static int r82xx_sysfreq_sel(struct r82xx_priv *priv, uint32_t freq,
			     enum r82xx_tuner_type type,
			     uint32_t delsys)
{
	int rc;
	uint8_t mixer_top, lna_top, cp_cur, div_buf_cur, lna_vth_l, mixer_vth_l;
	uint8_t air_cable1_in, cable2_in, pre_dect, lna_discharge, filter_cur;

	switch (delsys) {
	case SYS_DVBT:
		if ((freq == 506000000) || (freq == 666000000) ||
		   (freq == 818000000)) {
			mixer_top = 0x14;	/* mixer top:14 , top-1, low-discharge */
			lna_top = 0xe5;		/* detect bw 3, lna top:4, predet top:2 */
			cp_cur = 0x28;		/* 101, 0.2 */
			div_buf_cur = 0x20;	/* 10, 200u */
		} else {
			mixer_top = 0x24;	/* mixer top:13 , top-1, low-discharge */
			lna_top = 0xe5;		/* detect bw 3, lna top:4, predet top:2 */
			cp_cur = 0x38;		/* 111, auto */
			div_buf_cur = 0x30;	/* 11, 150u */
		}
		lna_vth_l = 0x53;		/* lna vth 0.84	,  vtl 0.64 */
		mixer_vth_l = 0x75;		/* mixer vth 1.04, vtl 0.84 */
		air_cable1_in = 0x00;
		cable2_in = 0x00;
		pre_dect = 0x40;
		lna_discharge = 14;
		filter_cur = 0x40;		/* 10, low */
		break;
	case SYS_DVBT2:
		mixer_top = 0x24;	/* mixer top:13 , top-1, low-discharge */
		lna_top = 0xe5;		/* detect bw 3, lna top:4, predet top:2 */
		lna_vth_l = 0x53;	/* lna vth 0.84	,  vtl 0.64 */
		mixer_vth_l = 0x75;	/* mixer vth 1.04, vtl 0.84 */
		air_cable1_in = 0x00;
		cable2_in = 0x00;
		pre_dect = 0x40;
		lna_discharge = 14;
		cp_cur = 0x38;		/* 111, auto */
		div_buf_cur = 0x30;	/* 11, 150u */
		filter_cur = 0x40;	/* 10, low */
		break;
	case SYS_ISDBT:
		mixer_top = 0x24;	/* mixer top:13 , top-1, low-discharge */
		lna_top = 0xe5;		/* detect bw 3, lna top:4, predet top:2 */
		lna_vth_l = 0x75;	/* lna vth 1.04	,  vtl 0.84 */
		mixer_vth_l = 0x75;	/* mixer vth 1.04, vtl 0.84 */
		air_cable1_in = 0x00;
		cable2_in = 0x00;
		pre_dect = 0x40;
		lna_discharge = 14;
		cp_cur = 0x38;		/* 111, auto */
		div_buf_cur = 0x30;	/* 11, 150u */
		filter_cur = 0x40;	/* 10, low */
		break;
	default: /* DVB-T 8M */
		mixer_top = 0x24;	/* mixer top:13 , top-1, low-discharge */
		lna_top = 0xe5;		/* detect bw 3, lna top:4, predet top:2 */
		lna_vth_l = 0x53;	/* lna vth 0.84	,  vtl 0.64 */
		mixer_vth_l = 0x75;	/* mixer vth 1.04, vtl 0.84 */
		air_cable1_in = 0x00;
		cable2_in = 0x00;
		pre_dect = 0x40;
		lna_discharge = 14;
		cp_cur = 0x38;		/* 111, auto */
		div_buf_cur = 0x30;	/* 11, 150u */
		filter_cur = 0x40;	/* 10, low */
		break;
	}

	if (priv->cfg->use_predetect) {
		rc = r82xx_write_reg_mask(priv, 0x06, pre_dect, 0x40);
		if (rc < 0)
			return rc;
	}

	rc = r82xx_write_reg_mask(priv, 0x1d, lna_top, 0xc7);
	if (rc < 0)
		return rc;
	rc = r82xx_write_reg_mask(priv, 0x1c, mixer_top, 0xf8);
	if (rc < 0)
		return rc;
	rc = r82xx_write_reg(priv, 0x0d, lna_vth_l);
	if (rc < 0)
		return rc;
	rc = r82xx_write_reg(priv, 0x0e, mixer_vth_l);
	if (rc < 0)
		return rc;

	priv->input = air_cable1_in;

	/* Air-IN only for Astrometa */
	rc = r82xx_write_reg_mask(priv, 0x05, air_cable1_in, 0x60);
	if (rc < 0)
		return rc;
	rc = r82xx_write_reg_mask(priv, 0x06, cable2_in, 0x08);
	if (rc < 0)
		return rc;

	rc = r82xx_write_reg_mask(priv, 0x11, cp_cur, 0x38);
	if (rc < 0)
		return rc;
	rc = r82xx_write_reg_mask(priv, 0x17, div_buf_cur, 0x30);
	if (rc < 0)
		return rc;
	rc = r82xx_write_reg_mask(priv, 0x0a, filter_cur, 0x60);
	if (rc < 0)
		return rc;

	/*
	 * Set LNA
	 */

	if (type != TUNER_ANALOG_TV) {
		/* LNA TOP: lowest */
		rc = r82xx_write_reg_mask(priv, 0x1d, 0, 0x38);
		if (rc < 0)
			return rc;

		/* 0: normal mode */
		rc = r82xx_write_reg_mask(priv, 0x1c, 0, 0x04);
		if (rc < 0)
			return rc;

		/* 0: PRE_DECT off */
		rc = r82xx_write_reg_mask(priv, 0x06, 0, 0x40);
		if (rc < 0)
			return rc;

		/* agc clk 250hz */
		rc = r82xx_write_reg_mask(priv, 0x1a, 0x30, 0x30);
		if (rc < 0)
			return rc;

//		msleep(250);

		/* write LNA TOP = 3 */
		rc = r82xx_write_reg_mask(priv, 0x1d, 0x18, 0x38);
		if (rc < 0)
			return rc;

		/*
		 * write discharge mode
		 * FIXME: IMHO, the mask here is wrong, but it matches
		 * what's there at the original driver
		 */
		rc = r82xx_write_reg_mask(priv, 0x1c, mixer_top, 0x04);
		if (rc < 0)
			return rc;

		/* LNA discharge current */
		rc = r82xx_write_reg_mask(priv, 0x1e, lna_discharge, 0x1f);
		if (rc < 0)
			return rc;

		/* agc clk 60hz */
		rc = r82xx_write_reg_mask(priv, 0x1a, 0x20, 0x30);
		if (rc < 0)
			return rc;
	} else {
		/* PRE_DECT off */
		rc = r82xx_write_reg_mask(priv, 0x06, 0, 0x40);
		if (rc < 0)
			return rc;

		/* write LNA TOP */
		rc = r82xx_write_reg_mask(priv, 0x1d, lna_top, 0x38);
		if (rc < 0)
			return rc;

		/*
		 * write discharge mode
		 * FIXME: IMHO, the mask here is wrong, but it matches
		 * what's there at the original driver
		 */
		rc = r82xx_write_reg_mask(priv, 0x1c, mixer_top, 0x04);
		if (rc < 0)
			return rc;

		/* LNA discharge current */
		rc = r82xx_write_reg_mask(priv, 0x1e, lna_discharge, 0x1f);
		if (rc < 0)
			return rc;

		/* agc clk 1Khz, external det1 cap 1u */
		rc = r82xx_write_reg_mask(priv, 0x1a, 0x00, 0x30);
		if (rc < 0)
			return rc;

		rc = r82xx_write_reg_mask(priv, 0x10, 0x00, 0x04);
		if (rc < 0)
			return rc;
	 }
	 return 0;
}

static int r82xx_set_tv_standard(struct r82xx_priv *priv,
				 unsigned bw,
				 enum r82xx_tuner_type type,
				 uint32_t delsys)

{
	int rc, i;
	uint32_t if_khz, filt_cal_lo;
	uint8_t data[5];
	uint8_t filt_gain, img_r, filt_q, hp_cor, ext_enable, loop_through;
	uint8_t lt_att, flt_ext_widest, polyfil_cur;
	int need_calibration;

	if (delsys == SYS_ISDBT) {
		if_khz = 4063;
		filt_cal_lo = 59000;
		filt_gain = 0x10;	/* +3db, 6mhz on */
		img_r = 0x00;		/* image negative */
		filt_q = 0x10;		/* r10[4]:low q(1'b1) */
		hp_cor = 0x6a;		/* 1.7m disable, +2cap, 1.25mhz */
		ext_enable = 0x40;	/* r30[6], ext enable; r30[5]:0 ext at lna max */
		loop_through = 0x00;	/* r5[7], lt on */
		lt_att = 0x00;		/* r31[7], lt att enable */
		flt_ext_widest = 0x00;	/* r15[7]: flt_ext_wide off */
		polyfil_cur = 0x60;	/* r25[6:5]:min */
	} else {
		if (bw <= 6) {
			if_khz = 3570;
			filt_cal_lo = 56000;	/* 52000->56000 */
			filt_gain = 0x10;	/* +3db, 6mhz on */
			img_r = 0x00;		/* image negative */
			filt_q = 0x10;		/* r10[4]:low q(1'b1) */
			hp_cor = 0x6b;		/* 1.7m disable, +2cap, 1.0mhz */
			ext_enable = 0x60;	/* r30[6]=1 ext enable; r30[5]:1 ext at lna max-1 */
			loop_through = 0x00;	/* r5[7], lt on */
			lt_att = 0x00;		/* r31[7], lt att enable */
			flt_ext_widest = 0x00;	/* r15[7]: flt_ext_wide off */
			polyfil_cur = 0x60;	/* r25[6:5]:min */
		} else if (bw == 7) {
#if 0
			/*
			 * There are two 7 MHz tables defined on the original
			 * driver, but just the second one seems to be visible
			 * by rtl2832. Keep this one here commented, as it
			 * might be needed in the future
			 */

			if_khz = 4070;
			filt_cal_lo = 60000;
			filt_gain = 0x10;	/* +3db, 6mhz on */
			img_r = 0x00;		/* image negative */
			filt_q = 0x10;		/* r10[4]:low q(1'b1) */
			hp_cor = 0x2b;		/* 1.7m disable, +1cap, 1.0mhz */
			ext_enable = 0x60;	/* r30[6]=1 ext enable; r30[5]:1 ext at lna max-1 */
			loop_through = 0x00;	/* r5[7], lt on */
			lt_att = 0x00;		/* r31[7], lt att enable */
			flt_ext_widest = 0x00;	/* r15[7]: flt_ext_wide off */
			polyfil_cur = 0x60;	/* r25[6:5]:min */
#endif
			/* 7 MHz, second table */
			if_khz = 4570;
			filt_cal_lo = 63000;
			filt_gain = 0x10;	/* +3db, 6mhz on */
			img_r = 0x00;		/* image negative */
			filt_q = 0x10;		/* r10[4]:low q(1'b1) */
			hp_cor = 0x2a;		/* 1.7m disable, +1cap, 1.25mhz */
			ext_enable = 0x60;	/* r30[6]=1 ext enable; r30[5]:1 ext at lna max-1 */
			loop_through = 0x00;	/* r5[7], lt on */
			lt_att = 0x00;		/* r31[7], lt att enable */
			flt_ext_widest = 0x00;	/* r15[7]: flt_ext_wide off */
			polyfil_cur = 0x60;	/* r25[6:5]:min */
		} else {
			if_khz = 4570;
			filt_cal_lo = 68500;
			filt_gain = 0x10;	/* +3db, 6mhz on */
			img_r = 0x00;		/* image negative */
			filt_q = 0x10;		/* r10[4]:low q(1'b1) */
			hp_cor = 0x0b;		/* 1.7m disable, +0cap, 1.0mhz */
			ext_enable = 0x60;	/* r30[6]=1 ext enable; r30[5]:1 ext at lna max-1 */
			loop_through = 0x00;	/* r5[7], lt on */
			lt_att = 0x00;		/* r31[7], lt att enable */
			flt_ext_widest = 0x00;	/* r15[7]: flt_ext_wide off */
			polyfil_cur = 0x60;	/* r25[6:5]:min */
		}
	}

	/* Initialize the shadow registers */
	memcpy(priv->regs, r82xx_init_array, sizeof(r82xx_init_array));

	/* Init Flag & Xtal_check Result (inits VGA gain, needed?)*/
	rc = r82xx_write_reg_mask(priv, 0x0c, 0x00, 0x0f);
	if (rc < 0)
		return rc;

	/* version */
	rc = r82xx_write_reg_mask(priv, 0x13, VER_NUM, 0x3f);
	if (rc < 0)
		return rc;

	/* for LT Gain test */
	if (type != TUNER_ANALOG_TV) {
		rc = r82xx_write_reg_mask(priv, 0x1d, 0x00, 0x38);
		if (rc < 0)
			return rc;
//		usleep_range(1000, 2000);
	}
	priv->int_freq = if_khz * 1000;

	/* Check if standard changed. If so, filter calibration is needed */
	/* as we call this function only once in rtlsdr, force calibration */
	need_calibration = 1;

	if (need_calibration) {
		for (i = 0; i < 2; i++) {
			/* Set filt_cap */
			rc = r82xx_write_reg_mask(priv, 0x0b, hp_cor, 0x60);
			if (rc < 0)
				return rc;

			/* set cali clk =on */
			rc = r82xx_write_reg_mask(priv, 0x0f, 0x04, 0x04);
			if (rc < 0)
				return rc;

			/* X'tal cap 0pF for PLL */
			rc = r82xx_write_reg_mask(priv, 0x10, 0x00, 0x03);
			if (rc < 0)
				return rc;

			rc = r82xx_set_pll(priv, filt_cal_lo * 1000);
			if (rc < 0 || !priv->has_lock)
				return rc;

			/* Start Trigger */
			rc = r82xx_write_reg_mask(priv, 0x0b, 0x10, 0x10);
			if (rc < 0)
				return rc;

//			usleep_range(1000, 2000);

			/* Stop Trigger */
			rc = r82xx_write_reg_mask(priv, 0x0b, 0x00, 0x10);
			if (rc < 0)
				return rc;

			/* set cali clk =off */
			rc = r82xx_write_reg_mask(priv, 0x0f, 0x00, 0x04);
			if (rc < 0)
				return rc;

			/* Check if calibration worked */
			rc = r82xx_read(priv, 0x00, data, sizeof(data));
			if (rc < 0)
				return rc;

			priv->fil_cal_code = data[4] & 0x0f;
			if (priv->fil_cal_code && priv->fil_cal_code != 0x0f)
				break;
		}
		/* narrowest */
		if (priv->fil_cal_code == 0x0f)
			priv->fil_cal_code = 0;
	}

	rc = r82xx_write_reg_mask(priv, 0x0a,
				  filt_q | priv->fil_cal_code, 0x1f);
	if (rc < 0)
		return rc;

	/* Set BW, Filter_gain, & HP corner */
	rc = r82xx_write_reg_mask(priv, 0x0b, hp_cor, 0xef);
	if (rc < 0)
		return rc;

	/* Set Img_R */
	rc = r82xx_write_reg_mask(priv, 0x07, img_r, 0x80);
	if (rc < 0)
		return rc;

	/* Set filt_3dB, V6MHz */
	rc = r82xx_write_reg_mask(priv, 0x06, filt_gain, 0x30);
	if (rc < 0)
		return rc;

	/* channel filter extension */
	rc = r82xx_write_reg_mask(priv, 0x1e, ext_enable, 0x60);
	if (rc < 0)
		return rc;

	/* Loop through */
	rc = r82xx_write_reg_mask(priv, 0x05, loop_through, 0x80);
	if (rc < 0)
		return rc;

	/* Loop through attenuation */
	rc = r82xx_write_reg_mask(priv, 0x1f, lt_att, 0x80);
	if (rc < 0)
		return rc;

	/* filter extension widest */
	rc = r82xx_write_reg_mask(priv, 0x0f, flt_ext_widest, 0x80);
	if (rc < 0)
		return rc;

	/* RF poly filter current */
	rc = r82xx_write_reg_mask(priv, 0x19, polyfil_cur, 0x60);
	if (rc < 0)
		return rc;

	/* Store current standard. If it changes, re-calibrate the tuner */
	priv->delsys = delsys;
	priv->type = type;
	priv->bw = bw;

	return 0;
}

static int r82xx_read_gain(struct r82xx_priv *priv)
{
	uint8_t data[4];
	int rc;

	rc = r82xx_read(priv, 0x00, data, sizeof(data));
	if (rc < 0)
		return rc;

	return ((data[3] & 0x0f) << 1) + ((data[3] & 0xf0) >> 4);
}

/* measured with a Racal 6103E GSM test set at 928 MHz with -60 dBm
 * input power, for raw results see:
 * http://steve-m.de/projects/rtl-sdr/gain_measurement/r820t/
 */

#define VGA_BASE_GAIN	-47
static const int r82xx_vga_gain_steps[]  = {
	0, 26, 26, 30, 42, 35, 24, 13, 14, 32, 36, 34, 35, 37, 35, 36
};

static const int r82xx_lna_gain_steps[]  = {
	0, 9, 13, 40, 38, 13, 31, 22, 26, 31, 26, 14, 19, 5, 35, 13
};

static const int r82xx_mixer_gain_steps[]  = {
	0, 5, 10, 10, 19, 9, 10, 25, 17, 10, 8, 16, 13, 6, 3, -8
};

int r82xx_set_gain(struct r82xx_priv *priv, int set_manual_gain, int gain)
{
	int rc;

	if (set_manual_gain) {
		int i, total_gain = 0;
		uint8_t mix_index = 0, lna_index = 0;
		uint8_t data[4];

		/* LNA auto off */
		rc = r82xx_write_reg_mask(priv, 0x05, 0x10, 0x10);
		if (rc < 0)
			return rc;

		 /* Mixer auto off */
		rc = r82xx_write_reg_mask(priv, 0x07, 0, 0x10);
		if (rc < 0)
			return rc;

		rc = r82xx_read(priv, 0x00, data, sizeof(data));
		if (rc < 0)
			return rc;

		/* set fixed VGA gain for now (16.3 dB) */
		rc = r82xx_write_reg_mask(priv, 0x0c, 0x08, 0x9f);
		if (rc < 0)
			return rc;

		for (i = 0; i < 15; i++) {
			if (total_gain >= gain)
				break;

			total_gain += r82xx_lna_gain_steps[++lna_index];

			if (total_gain >= gain)
				break;

			total_gain += r82xx_mixer_gain_steps[++mix_index];
		}

		/* set LNA gain */
		rc = r82xx_write_reg_mask(priv, 0x05, lna_index, 0x0f);
		if (rc < 0)
			return rc;

		/* set Mixer gain */
		rc = r82xx_write_reg_mask(priv, 0x07, mix_index, 0x0f);
		if (rc < 0)
			return rc;
	} else {
		/* LNA */
		rc = r82xx_write_reg_mask(priv, 0x05, 0, 0x10);
		if (rc < 0)
			return rc;

		/* Mixer */
		rc = r82xx_write_reg_mask(priv, 0x07, 0x10, 0x10);
		if (rc < 0)
			return rc;

		/* set fixed VGA gain for now (26.5 dB) */
		rc = r82xx_write_reg_mask(priv, 0x0c, 0x0b, 0x9f);
		if (rc < 0)
			return rc;
	}

	return 0;
}

/* Bandwidth contribution by low-pass filter. */
static const int r82xx_if_low_pass_bw_table[] = {
	1700000, 1600000, 1550000, 1450000, 1200000, 900000, 700000, 550000, 450000, 350000
};

#define FILT_HP_BW1 350000
#define FILT_HP_BW2 380000
int r82xx_set_bandwidth(struct r82xx_priv *priv, int bw, uint32_t rate)
{
	int rc;
	unsigned int i;
	int real_bw = 0;
	uint8_t reg_0a;
	uint8_t reg_0b;

	if (bw > 7000000) {
		// BW: 8 MHz
		reg_0a = 0x10;
		reg_0b = 0x0b;
		priv->int_freq = 4570000;
	} else if (bw > 6000000) {
		// BW: 7 MHz
		reg_0a = 0x10;
		reg_0b = 0x2a;
		priv->int_freq = 4570000;
	} else if (bw > r82xx_if_low_pass_bw_table[0] + FILT_HP_BW1 + FILT_HP_BW2) {
		// BW: 6 MHz
		reg_0a = 0x10;
		reg_0b = 0x6b;
		priv->int_freq = 3570000;
	} else {
		reg_0a = 0x00;
		reg_0b = 0x80;
		priv->int_freq = 2300000;

		if (bw > r82xx_if_low_pass_bw_table[0] + FILT_HP_BW1) {
			bw -= FILT_HP_BW2;
			priv->int_freq += FILT_HP_BW2;
			real_bw += FILT_HP_BW2;
		} else {
			reg_0b |= 0x20;
		}

		if (bw > r82xx_if_low_pass_bw_table[0]) {
			bw -= FILT_HP_BW1;
			priv->int_freq += FILT_HP_BW1;
			real_bw += FILT_HP_BW1;
		} else {
			reg_0b |= 0x40;
		}

		// find low-pass filter
		for(i = 0; i < ARRAY_SIZE(r82xx_if_low_pass_bw_table); ++i) {
			if (bw > r82xx_if_low_pass_bw_table[i])
				break;
		}
		--i;
		reg_0b |= 15 - i;
		real_bw += r82xx_if_low_pass_bw_table[i];

		priv->int_freq -= real_bw / 2;
	}

	rc = r82xx_write_reg_mask(priv, 0x0a, reg_0a, 0x10);
	if (rc < 0)
		return rc;

	rc = r82xx_write_reg_mask(priv, 0x0b, reg_0b, 0xef);
	if (rc < 0)
		return rc;

	return priv->int_freq;
}
#undef FILT_HP_BW1
#undef FILT_HP_BW2

int r82xx_set_freq(struct r82xx_priv *priv, uint32_t freq)
{
	int rc = -1;
	uint32_t lo_freq = freq + priv->int_freq;
	uint8_t air_cable1_in;

	rc = r82xx_set_mux(priv, lo_freq);
	if (rc < 0)
		goto err;

	rc = r82xx_set_pll(priv, lo_freq);
	if (rc < 0 || !priv->has_lock)
		goto err;

	/* switch between 'Cable1' and 'Air-In' inputs on sticks with
	 * R828D tuner. We switch at 345 MHz, because that's where the
	 * noise-floor has about the same level with identical LNA
	 * settings. The original driver used 320 MHz. */
	air_cable1_in = (freq > MHZ(345)) ? 0x00 : 0x60;

	if ((priv->cfg->rafael_chip == CHIP_R828D) &&
	    (air_cable1_in != priv->input)) {
		priv->input = air_cable1_in;
		rc = r82xx_write_reg_mask(priv, 0x05, air_cable1_in, 0x60);
	}

err:
	if (rc < 0)
		fprintf(stderr, "%s: failed=%d\n", __FUNCTION__, rc);
	return rc;
}

/*
 * r82xx standby logic
 */

int r82xx_standby(struct r82xx_priv *priv)
{
	int rc;

	/* If device was not initialized yet, don't need to standby */
	if (!priv->init_done)
		return 0;

	rc = r82xx_write_reg(priv, 0x06, 0xb1);
	if (rc < 0)
		return rc;
	rc = r82xx_write_reg(priv, 0x05, 0x03);
	if (rc < 0)
		return rc;
	rc = r82xx_write_reg(priv, 0x07, 0x3a);
	if (rc < 0)
		return rc;
	rc = r82xx_write_reg(priv, 0x08, 0x40);
	if (rc < 0)
		return rc;
	rc = r82xx_write_reg(priv, 0x09, 0xc0);
	if (rc < 0)
		return rc;
	rc = r82xx_write_reg(priv, 0x0a, 0x36);
	if (rc < 0)
		return rc;
	rc = r82xx_write_reg(priv, 0x0c, 0x35);
	if (rc < 0)
		return rc;
	rc = r82xx_write_reg(priv, 0x0f, 0x68);
	if (rc < 0)
		return rc;
	rc = r82xx_write_reg(priv, 0x11, 0x03);
	if (rc < 0)
		return rc;
	rc = r82xx_write_reg(priv, 0x17, 0xf4);
	if (rc < 0)
		return rc;
	rc = r82xx_write_reg(priv, 0x19, 0x0c);

	/* Force initial calibration */
	priv->type = -1;

	return rc;
}

/*
 * r82xx device init logic
 */

static int r82xx_xtal_check(struct r82xx_priv *priv)
{
	int rc;
	unsigned int i;
	uint8_t data[3], val;

	/* Initialize the shadow registers */
	memcpy(priv->regs, r82xx_init_array, sizeof(r82xx_init_array));

	/* cap 30pF & Drive Low */
	rc = r82xx_write_reg_mask(priv, 0x10, 0x0b, 0x0b);
	if (rc < 0)
		return rc;

	/* set pll autotune = 128kHz */
	rc = r82xx_write_reg_mask(priv, 0x1a, 0x00, 0x0c);
	if (rc < 0)
		return rc;

	/* set manual initial reg = 111111;  */
	rc = r82xx_write_reg_mask(priv, 0x13, 0x7f, 0x7f);
	if (rc < 0)
		return rc;

	/* set auto */
	rc = r82xx_write_reg_mask(priv, 0x13, 0x00, 0x40);
	if (rc < 0)
		return rc;

	/* Try several xtal capacitor alternatives */
	for (i = 0; i < ARRAY_SIZE(r82xx_xtal_capacitor); i++) {
		rc = r82xx_write_reg_mask(priv, 0x10,
					  r82xx_xtal_capacitor[i][0], 0x1b);
		if (rc < 0)
			return rc;

//		usleep_range(5000, 6000);

		rc = r82xx_read(priv, 0x00, data, sizeof(data));
		if (rc < 0)
			return rc;
		if (!(data[2] & 0x40))
			continue;

		val = data[2] & 0x3f;

		if (priv->cfg->xtal == 16000000 && (val > 29 || val < 23))
			break;

		if (val != 0x3f)
			break;
	}

	if (i == ARRAY_SIZE(r82xx_xtal_capacitor))
		return -1;

	return r82xx_xtal_capacitor[i][1];
}

int r82xx_init(struct r82xx_priv *priv)
{
	int rc;

	/* TODO: R828D might need r82xx_xtal_check() */
	priv->xtal_cap_sel = XTAL_HIGH_CAP_0P;

	/* Initialize registers */
	rc = r82xx_write(priv, 0x05,
			 r82xx_init_array, sizeof(r82xx_init_array));

	rc = r82xx_set_tv_standard(priv, 3, TUNER_DIGITAL_TV, 0);
	if (rc < 0)
		goto err;

	rc = r82xx_sysfreq_sel(priv, 0, TUNER_DIGITAL_TV, SYS_DVBT);

	priv->init_done = 1;

err:
	if (rc < 0)
		fprintf(stderr, "%s: failed=%d\n", __FUNCTION__, rc);
	return rc;
}

#if 0
/* Not used, for now */
static int r82xx_gpio(struct r82xx_priv *priv, int enable)
{
	return r82xx_write_reg_mask(priv, 0x0f, enable ? 1 : 0, 0x01);
}
#endif
