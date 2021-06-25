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

#ifndef R82XX_H
#define R82XX_H

#define R820T_I2C_ADDR		0x34
#define R828D_I2C_ADDR		0x74
#define R828D_XTAL_FREQ		16000000

#define R82XX_CHECK_ADDR	0x00
#define R82XX_CHECK_VAL		0x69

#define R82XX_IF_FREQ		3570000

#define REG_SHADOW_START	5
#define NUM_REGS		30
#define NUM_IMR			5
#define IMR_TRIAL		9

#define VER_NUM			49

enum r82xx_chip {
	CHIP_R820T,
	CHIP_R620D,
	CHIP_R828D,
	CHIP_R828,
	CHIP_R828S,
	CHIP_R820C,
};

enum r82xx_tuner_type {
	TUNER_RADIO = 1,
	TUNER_ANALOG_TV,
	TUNER_DIGITAL_TV
};

enum r82xx_xtal_cap_value {
	XTAL_LOW_CAP_30P = 0,
	XTAL_LOW_CAP_20P,
	XTAL_LOW_CAP_10P,
	XTAL_LOW_CAP_0P,
	XTAL_HIGH_CAP_0P
};

struct r82xx_config {
	uint8_t i2c_addr;
	uint32_t xtal;
	enum r82xx_chip rafael_chip;
	unsigned int max_i2c_msg_len;
	int use_predetect;
};

struct r82xx_priv {
	struct r82xx_config		*cfg;

	uint8_t				regs[NUM_REGS];
	uint8_t				buf[NUM_REGS + 1];
	enum r82xx_xtal_cap_value	xtal_cap_sel;
	uint16_t			pll;	/* kHz */
	uint32_t			int_freq;
	uint8_t				fil_cal_code;
	uint8_t				input;
	int				has_lock;
	int				init_done;

	/* Store current mode */
	uint32_t			delsys;
	enum r82xx_tuner_type		type;

	uint32_t			bw;	/* in MHz */

	void *rtl_dev;
};

struct r82xx_freq_range {
	uint32_t	freq;
	uint8_t		open_d;
	uint8_t		rf_mux_ploy;
	uint8_t		tf_c;
	uint8_t		xtal_cap20p;
	uint8_t		xtal_cap10p;
	uint8_t		xtal_cap0p;
};

enum r82xx_delivery_system {
	SYS_UNDEFINED,
	SYS_DVBT,
	SYS_DVBT2,
	SYS_ISDBT,
};

int r82xx_standby(struct r82xx_priv *priv);
int r82xx_init(struct r82xx_priv *priv);
int r82xx_set_freq(struct r82xx_priv *priv, uint32_t freq);
int r82xx_set_gain(struct r82xx_priv *priv, int set_manual_gain, int gain);
int r82xx_set_bandwidth(struct r82xx_priv *priv, int bandwidth,  uint32_t rate);

#endif
