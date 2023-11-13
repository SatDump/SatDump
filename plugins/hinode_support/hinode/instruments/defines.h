#pragma once

#define MAX_RST 4096

#define MAX_PIXELS (256 * 1024)
#define MAX_COMP_LEN (MAX_PIXELS * 4)

#define COMP_MODE_DPCM12 3
#define COMP_MODE_DCT12 7

#define ALIGN 0x10000
#define MAX_SQDIFF 999

#define RST2ND_MASK 0xF8
#define RSTNO_MASK 0x07

#define BUFF_NOW 0
#define JPEG_EXTRACT 1
#define JPEG_RESTORED 2
#define RESTORED_IRRETRIEVABLE 3
#define NOT_APID -1
#define NOT_JPEG_DATA -2
#define JPEG_IRRETRIEVABLE -3
#define JPEG_INIT -4
