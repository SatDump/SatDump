/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifndef LA_MIAM_CORE_H
#define LA_MIAM_CORE_H 1

#include <stdint.h>
#include <aero/libacars/libacars.h>          // la_type_descriptor, la_proto_node
#include <aero/libacars/vstring.h>           // la_vstring

#ifdef __cplusplus
extern "C" {
#endif

// MIAM decoder error codes. Expressed as uint32_t:
// Lower half: header errors
// Upper half: body errors
#define LA_MIAM_ERR_SUCCESS                 (0)
#define LA_MIAM_ERR_HDR_PDU_TYPE_UNKNOWN    (1 << 1)
#define LA_MIAM_ERR_HDR_PDU_VERSION_UNKNOWN (1 << 2)
#define LA_MIAM_ERR_HDR_TRUNCATED           (1 << 3)
#define LA_MIAM_ERR_HDR_APP_TYPE_UNKNOWN    (1 << 4)

#define LA_MIAM_ERR_BODY_TRUNCATED          (1 << 16)
#define LA_MIAM_ERR_BODY_INFLATE_FAILED     (1 << 17)
#define LA_MIAM_ERR_BODY_COMPR_UNSUPPORTED  (1 << 18)
#define LA_MIAM_ERR_BODY_CRC_FAILED         (1 << 19)

// header/body error masks
#define LA_MIAM_ERR_HDR                     0x0000ffffu
#define LA_MIAM_ERR_BODY                    0xffff0000u

typedef struct {
	void *data;
	uint32_t pdu_len;
	uint32_t data_len;
	uint32_t err;
	uint32_t crc;
	char aircraft_id[8];
	char app_id[7];
	uint8_t msg_num;
	uint8_t ack_option;
	uint8_t compression;
	uint8_t encoding;
	uint8_t app_type;
	// reserved for future use
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
	void (*reserved4)(void);
	void (*reserved5)(void);
	void (*reserved6)(void);
	void (*reserved7)(void);
	void (*reserved8)(void);
	void (*reserved9)(void);
} la_miam_core_v1_data_pdu;

typedef struct {
	void *data;
	uint32_t err;
	uint32_t pdu_len;
	uint32_t data_len;
	char aircraft_id[8];
	uint8_t msg_ack_num;
	uint8_t ack_xfer_result;
	uint8_t crc[4];
	// reserved for future use
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
	void (*reserved4)(void);
	void (*reserved5)(void);
	void (*reserved6)(void);
	void (*reserved7)(void);
	void (*reserved8)(void);
	void (*reserved9)(void);
} la_miam_core_v1_ack_pdu;

typedef struct {
	void *data;
	uint32_t err;
	uint32_t data_len;
	uint16_t crc;
	char app_id[9];
	uint8_t msg_num;
	uint8_t ack_option;
	uint8_t compression;
	uint8_t encoding;
	uint8_t app_type;
	// reserved for future use
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
	void (*reserved4)(void);
	void (*reserved5)(void);
	void (*reserved6)(void);
	void (*reserved7)(void);
	void (*reserved8)(void);
	void (*reserved9)(void);
} la_miam_core_v2_data_pdu;

typedef struct {
	void *data;
	uint32_t err;
	uint32_t data_len;
	uint8_t msg_ack_num;
	uint8_t ack_xfer_result;
	uint8_t crc[2];
	// reserved for future use
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
	void (*reserved4)(void);
	void (*reserved5)(void);
	void (*reserved6)(void);
	void (*reserved7)(void);
	void (*reserved8)(void);
	void (*reserved9)(void);
} la_miam_core_v2_ack_pdu;

typedef struct {
	void *data;
	uint32_t err;
	uint32_t pdu_len;
	uint32_t data_len;
	char aircraft_id[8];
	uint8_t compression;
	uint8_t networks;
	// reserved for future use
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
	void (*reserved4)(void);
	void (*reserved5)(void);
	void (*reserved6)(void);
	void (*reserved7)(void);
	void (*reserved8)(void);
	void (*reserved9)(void);
} la_miam_core_v1v2_alo_alr_pdu;

#define LA_MIAM_CORE_VER_MAX 2

typedef enum {
	LA_MIAM_CORE_PDU_DATA = 0,
	LA_MIAM_CORE_PDU_ACK = 1,
	LA_MIAM_CORE_PDU_ALO = 2,
	LA_MIAM_CORE_PDU_ALR = 3,
	LA_MIAM_CORE_PDU_UNKNOWN = 4
} la_miam_core_pdu_type;
#define LA_MIAM_CORE_PDU_TYPE_MAX 4

typedef struct {
	uint32_t err;                   // PDU decoding error code
	uint8_t version;                // MIAM CORE PDU version
	la_miam_core_pdu_type pdu_type; // MIAM CORE PDU type
	// reserved for future use
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
	void (*reserved4)(void);
	void (*reserved5)(void);
	void (*reserved6)(void);
	void (*reserved7)(void);
	void (*reserved8)(void);
	void (*reserved9)(void);
} la_miam_core_pdu;

// miam-core.c
la_proto_node *la_miam_core_pdu_parse(char const *txt);
void la_miam_core_format_text(la_vstring *vstr, void const *data, int indent);
void la_miam_core_format_json(la_vstring *vstr, void const *data);
la_proto_node *la_proto_tree_find_miam_core(la_proto_node *root);

extern la_type_descriptor const la_DEF_miam_core_pdu;
extern la_type_descriptor const la_DEF_miam_core_v1v2_alo_pdu;
extern la_type_descriptor const la_DEF_miam_core_v1v2_alr_pdu;

extern la_type_descriptor const la_DEF_miam_core_v1_data_pdu;
extern la_type_descriptor const la_DEF_miam_core_v1_ack_pdu;

extern la_type_descriptor const la_DEF_miam_core_v2_data_pdu;
extern la_type_descriptor const la_DEF_miam_core_v2_ack_pdu;

#ifdef __cplusplus
}
#endif

#endif // !LA_MIAM_CORE_H
