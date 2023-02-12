/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifndef LA_ADSC_H
#define LA_ADSC_H 1

#include <stdbool.h>
#include <stdint.h>
#include <aero/libacars/libacars.h>      // la_proto_node, la_type_descriptor
#include <aero/libacars/arinc.h>         // la_arinc_imi
#include <aero/libacars/list.h>          // la_list
#include <aero/libacars/vstring.h>       // la_vstring

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	la_vstring *vstr;
	int indent;
} la_adsc_formatter_ctx_t;

typedef int(la_adsc_parser_fun)(void *dest, uint8_t const *buf, uint32_t len);
typedef void(la_adsc_formatter_fun)(la_adsc_formatter_ctx_t *ctx, char const *label, void const *data);
typedef void(la_adsc_destructor_fun)(void *data);

typedef struct {
	char const *label;
	char const *json_key;
	la_adsc_parser_fun *parse;
	la_adsc_formatter_fun *format_text;
	la_adsc_formatter_fun *format_json;
	la_adsc_destructor_fun *destroy;
	// reserved for future use
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
} la_adsc_type_descriptor_t;

// ADS-C message
typedef struct {
	bool err;
	la_list *tag_list;
	// reserved for future use
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
} la_adsc_msg_t;

// generic tag structure
typedef struct {
	uint8_t tag;
	la_adsc_type_descriptor_t *type;
	void *data;
} la_adsc_tag_t;

// Downlink tag structures

// negative acknowledgement (tag 4)
typedef struct {
	uint8_t contract_req_num;
	uint8_t reason;
	uint8_t ext_data;
} la_adsc_nack_t;
#define LA_ADSC_NACK_MAX_REASON_CODE 13

// description of a single non-compliant message group contained in the above notification
typedef struct {
	uint8_t noncomp_tag;                /* non-compliant tag value */
	uint8_t is_unrecognized;            /* 1 - group unrecognized, 0 - group unavailable */
	uint8_t is_whole_group_unavail;     /* 1 - entire group is unavailable
	                                       0 - one or more group params is unavailable */
	uint8_t param_cnt;                  /* number of unavailable params
	                                       (used when is_whole_group_noncompliant==0) */
	uint8_t params[15];                 /* a table of non-compliant parameter numbers */
} la_adsc_noncomp_group_t;

// noncompliance notification (tag 5)
typedef struct {
	uint8_t contract_req_num;           // contract request number
	uint8_t group_cnt;                  // number of non-compliant groups
	la_adsc_noncomp_group_t *groups;    // a table of non-compliant groups
} la_adsc_noncomp_notify_t;

// basic ADS group (downlink tags: 7, 9, 10, 18, 19, 20)
typedef struct {
	double lat, lon;
	double timestamp;
	int alt;
	uint8_t redundancy, accuracy, tcas_health;
} la_adsc_basic_report_t;

// flight ID group (tag 12)
typedef struct {
	char id[9];
} la_adsc_flight_id_t;

// predicted route group (tag 13)
typedef struct {
	double lat_next, lon_next;
	double lat_next_next, lon_next_next;
	int alt_next, alt_next_next;
	int eta_next;
} la_adsc_predicted_route_t;

// earth or air reference group (tags: 14, 15)
typedef struct {
	double heading;
	double speed;
	int vert_speed;
	uint8_t heading_invalid;
} la_adsc_earth_air_ref_t;

// meteorological group (tag 16)
typedef struct {
	double wind_speed;
	double wind_dir;
	double temp;
	uint8_t wind_dir_invalid;
} la_adsc_meteo_t;

// airframe ID group (tag 17)
typedef struct {
	uint8_t icao_hex[3];
} la_adsc_airframe_id_t;

// intermediate projected intent group (tag 22)
typedef struct {
	double distance;
	double track;
	int alt;
	int eta;
	uint8_t track_invalid;
} la_adsc_intermediate_projection_t;

// fixed projected intent group (tag 23)
typedef struct {
	double lat, lon;
	int alt;
	int eta;
} la_adsc_fixed_projection_t;

// Uplink tag structures

// periodic and event contract requests (tags: 7, 8, 9)
typedef struct {
	uint8_t contract_num;           // contract number
	la_list *req_tag_list;          // list of la_adsc_tag_t's describing requested report groups
} la_adsc_req_t;

// lateral deviation change (uplink tag 10)
typedef struct {
	double lat_dev_threshold;
} la_adsc_lat_dev_chg_event_t;

// reporting interval (uplink tag 11)
typedef struct {
	uint8_t scaling_factor;
	uint8_t rate;
} la_adsc_report_interval_req_t;

// vertical speed change threshold (uplink tag 18)
typedef struct {
	int vspd_threshold;
} la_adsc_vspd_chg_event_t;

// altitude range change event (uplink tag 19)
typedef struct {
	int ceiling_alt, floor_alt;
} la_adsc_alt_range_event_t;

// aircraft intent group (uplink tag 21)
typedef struct {
	uint8_t modulus;
	uint8_t acft_intent_projection_time;
} la_adsc_acft_intent_group_req_t;

// adsc.c
extern la_type_descriptor const la_DEF_adsc_message;
la_proto_node *la_adsc_parse(uint8_t const *buf, int len, la_msg_dir msg_dir, la_arinc_imi imi);
void la_adsc_format_text(la_vstring *vstr, void const *data, int indent);
void la_adsc_format_json(la_vstring *vstr, void const *data);
void la_adsc_destroy(void *data);
la_proto_node *la_proto_tree_find_adsc(la_proto_node *root);

#ifdef __cplusplus
}
#endif

#endif // !LA_ADSC_H
