/*
Copyright (c) 2013-2017, Youssef Touil <youssef@airspy.com>
Copyright (c) 2013-2017, Ian Gilmour <ian@sdrsharp.com>
Copyright (c) 2013-2017, Benjamin Vernoux <bvernoux@airspy.com>

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

#ifndef __AIRSPYHF_COMMANDS_H__
#define __AIRSPYHF_COMMANDS_H__

#include <stdint.h>

typedef enum
{
    RECEIVER_MODE_OFF                   = 0,
    RECEIVER_MODE_ON                    = 1
} receiver_mode_t;

#define AIRSPYHF_CMD_MAX (15)
typedef enum
{
    AIRSPYHF_INVALID                      = 0,
    AIRSPYHF_RECEIVER_MODE                = 1,
    AIRSPYHF_SET_FREQ                     = 2,
    AIRSPYHF_GET_SAMPLERATES              = 3,
    AIRSPYHF_SET_SAMPLERATE               = 4,
    AIRSPYHF_CONFIG_READ                  = 5,
    AIRSPYHF_CONFIG_WRITE                 = 6,
    AIRSPYHF_GET_SERIALNO_BOARDID         = 7,
	AIRSPYHF_SET_USER_OUTPUT              = 8,
	AIRSPYHF_GET_VERSION_STRING           = 9,
	AIRSPYHF_SET_HF_AGC                   = 10,
	AIRSPYHF_SET_HF_AGC_THRESHOLD         = 11,
	AIRSPYHF_SET_HF_ATT                   = 12,
	AIRSPYHF_SET_HF_LNA                   = 13,
	AIRSPYHF_GET_SAMPLERATE_ARCHITECTURES = 14,
	AIRSPYHF_GET_FILTER_GAIN              = AIRSPYHF_CMD_MAX,
} airspyhf_vendor_request;

#endif
