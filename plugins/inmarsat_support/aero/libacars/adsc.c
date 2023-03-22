/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#include <stdint.h>
#include <string.h>                 // memcpy()
#include <math.h>                   // pow(), trunc()
#include <aero/libacars/macros.h>        // la_debug_print
#include <aero/libacars/dict.h>          // la_dict
#include <aero/libacars/bitstream.h>     // la_bitstream_*
#include <aero/libacars/libacars.h>      // la_msg_dir, la_proto_node, la_proto_tree_find_protocol
#include <aero/libacars/arinc.h>         // la_arinc_imi
#include <aero/libacars/list.h>          // la_list_*
#include <aero/libacars/util.h>          // la_dict, la_dict_search(), LA_XCALLOC, LA_XFREE
#include <aero/libacars/vstring.h>       // la_vstring, la_vstring_append_sprintf()
#include <aero/libacars/json.h>          // la_json_object_*(), la_json_append_*()
#include <aero/libacars/adsc.h>

static double la_adsc_coordinate_parse(uint32_t c) {
	// extend the 21-bit signed field to 32-bit signed int
	struct { signed int coord:21; } s;
	int r = s.coord = (int)c;
	la_debug_print(D_INFO, "r=%d\n", r);
	// Field range is -180 to 180 degrees.
	// MSB weight is defined to have a weight of 90 degrees.
	// LSB weight is therefore 90/(2^19).
	// First, calculate maximum value of the field:
	double result = (180.0-90.0/pow(2, 19));
	// Then multiply it by r/max_r
	result *= (double)r;
	result /= (double)0xfffff;
	la_debug_print(D_INFO, "result: %f\n", result);
	return result;
}

static int la_adsc_altitude_parse(uint32_t a) {
	struct { signed int alt:16; } s;
	int result = s.alt = (int)a;
	result *= 4;
	la_debug_print(D_INFO, "result: %d\n", result);
	return result;
}

static double la_adsc_timestamp_parse(uint32_t t) {
	double result = (double)t * 0.125;
	la_debug_print(D_INFO, "result: %f\n", result);
	return result;
}

static double la_adsc_speed_parse(uint32_t s) {
	double result = (double)s / 2.0;
	la_debug_print(D_INFO, "result: %f\n", result);
	return result;
}

static int la_adsc_vert_speed_parse(uint32_t vs) {
	struct { signed int vs:12; } s;
	int result = s.vs = (int)vs;
	result *= 16;
	la_debug_print(D_INFO, "result: %d\n", result);
	return result;
}

static double la_adsc_distance_parse(uint32_t d) {
	double result = (double)d / 8.0;
	la_debug_print(D_INFO, "result: %f\n", result);
	return result;
}

static double la_adsc_heading_parse(uint32_t h) {
	// Heading/track format is the same as latitude/longitude
	// except that:
	// - the field is 12-bit long (including sign bit)
	// - LSB weight is 90/(2^10).
	// FIXME: reduce this to a common function
	struct { signed int hdg:12; } s;
	int r = s.hdg = (int)h;
	la_debug_print(D_INFO, "r=%d\n", r);
	double result = (180.0-90.0/pow(2, 10));
	result *= (double)r;
	result /= (double)0x7ff;
	if(result < 0.0)
		result += 360.0;
	la_debug_print(D_INFO, "result: %f\n", result);
	return result;
}

static double la_adsc_wind_dir_parse(uint32_t w) {
	// Wind direction format is the same as latitude/longitude
	// except that:
	// - the field is 9-bit long (including sign bit)
	// - LSB weight is 90/(2^7).
	struct { signed int dir:9; } s;
	int r = s.dir = (int)w;
	la_debug_print(D_INFO, "r=%d\n", r);
	double result = (180.0-90.0/pow(2, 7));
	result *= (double)r;
	result /= (double)0xff;
	if(result < 0.0)
		result += 360.0;
	la_debug_print(D_INFO, "result: %f\n", result);
	return result;
}

static double la_adsc_temperature_parse(uint32_t t) {
	struct { signed int temp:12; } s;
	int r = s.temp = (int)t;
	la_debug_print(D_INFO, "r=%d\n", r);
	double result = (512.0-256.0/pow(2, 10));
	result *= (double)r;
	result /= (double)0x7ff;
	la_debug_print(D_INFO, "result: %f\n", result);
	return result;
}

#define LA_ADSC_CHECK_LEN(t, l, m) if((l) < (m)) { \
	la_debug_print(D_ERROR, "Truncated tag %u: len: %u < %u\n", (t), (l), (m)); \
	return -1; \
}
#define LA_ADSC_PARSER_FUN(x) static int x(void *dest, uint8_t const *buf, uint32_t len)
#define LA_ADSC_FORMATTER_FUN(x) static void x(la_adsc_formatter_ctx_t *ctx, char const *label, void const *data)

static int la_adsc_tag_parse(la_adsc_tag_t *t, la_dict const
		*tag_descriptor_table, uint8_t const *buf, uint32_t len);

/***************************************************
 * Prototypes of functions used in descriptor tables
 ***************************************************/

LA_ADSC_PARSER_FUN(la_adsc_uint8_t_parse);
LA_ADSC_PARSER_FUN(la_adsc_contract_request_parse);
LA_ADSC_PARSER_FUN(la_adsc_reporting_interval_parse);
LA_ADSC_PARSER_FUN(la_adsc_lat_dev_change_parse);
LA_ADSC_PARSER_FUN(la_adsc_vspd_change_parse);
LA_ADSC_PARSER_FUN(la_adsc_alt_range_parse);
LA_ADSC_PARSER_FUN(la_adsc_acft_intent_group_parse);
LA_ADSC_PARSER_FUN(la_adsc_nack_parse);
LA_ADSC_PARSER_FUN(la_adsc_noncomp_notify_parse);
LA_ADSC_PARSER_FUN(la_adsc_basic_report_parse);
LA_ADSC_PARSER_FUN(la_adsc_flight_id_parse);
LA_ADSC_PARSER_FUN(la_adsc_predicted_route_parse);
LA_ADSC_PARSER_FUN(la_adsc_earth_air_ref_parse);
LA_ADSC_PARSER_FUN(la_adsc_intermediate_projection_parse);
LA_ADSC_PARSER_FUN(la_adsc_fixed_projection_parse);
LA_ADSC_PARSER_FUN(la_adsc_meteo_parse);
LA_ADSC_PARSER_FUN(la_adsc_airframe_id_parse);

LA_ADSC_FORMATTER_FUN(la_adsc_empty_tag_format_text);
LA_ADSC_FORMATTER_FUN(la_adsc_tag_with_contract_number_format_text);
LA_ADSC_FORMATTER_FUN(la_adsc_contract_request_format_text);
LA_ADSC_FORMATTER_FUN(la_adsc_reporting_interval_format_text);
LA_ADSC_FORMATTER_FUN(la_adsc_lat_dev_change_format_text);
LA_ADSC_FORMATTER_FUN(la_adsc_vspd_change_format_text);
LA_ADSC_FORMATTER_FUN(la_adsc_alt_range_format_text);
LA_ADSC_FORMATTER_FUN(la_adsc_acft_intent_group_format_text);
LA_ADSC_FORMATTER_FUN(la_adsc_modulus_format_text);
LA_ADSC_FORMATTER_FUN(la_adsc_nack_format_text);
LA_ADSC_FORMATTER_FUN(la_adsc_dis_reason_code_format_text);
LA_ADSC_FORMATTER_FUN(la_adsc_noncomp_notify_format_text);
LA_ADSC_FORMATTER_FUN(la_adsc_basic_report_format_text);
LA_ADSC_FORMATTER_FUN(la_adsc_flight_id_format_text);
LA_ADSC_FORMATTER_FUN(la_adsc_predicted_route_format_text);
LA_ADSC_FORMATTER_FUN(la_adsc_earth_ref_format_text);
LA_ADSC_FORMATTER_FUN(la_adsc_air_ref_format_text);
LA_ADSC_FORMATTER_FUN(la_adsc_intermediate_projection_format_text);
LA_ADSC_FORMATTER_FUN(la_adsc_fixed_projection_format_text);
LA_ADSC_FORMATTER_FUN(la_adsc_meteo_format_text);
LA_ADSC_FORMATTER_FUN(la_adsc_airframe_id_format_text);

LA_ADSC_FORMATTER_FUN(la_adsc_empty_tag_format_json);
LA_ADSC_FORMATTER_FUN(la_adsc_tag_with_contract_number_format_json);
LA_ADSC_FORMATTER_FUN(la_adsc_contract_request_format_json);
LA_ADSC_FORMATTER_FUN(la_adsc_reporting_interval_format_json);
LA_ADSC_FORMATTER_FUN(la_adsc_lat_dev_change_format_json);
LA_ADSC_FORMATTER_FUN(la_adsc_vspd_change_format_json);
LA_ADSC_FORMATTER_FUN(la_adsc_alt_range_format_json);
LA_ADSC_FORMATTER_FUN(la_adsc_acft_intent_group_format_json);
LA_ADSC_FORMATTER_FUN(la_adsc_modulus_format_json);
LA_ADSC_FORMATTER_FUN(la_adsc_nack_format_json);
LA_ADSC_FORMATTER_FUN(la_adsc_dis_reason_code_format_json);
LA_ADSC_FORMATTER_FUN(la_adsc_noncomp_notify_format_json);
LA_ADSC_FORMATTER_FUN(la_adsc_basic_report_format_json);
LA_ADSC_FORMATTER_FUN(la_adsc_flight_id_format_json);
LA_ADSC_FORMATTER_FUN(la_adsc_predicted_route_format_json);
LA_ADSC_FORMATTER_FUN(la_adsc_earth_ref_format_json);
LA_ADSC_FORMATTER_FUN(la_adsc_air_ref_format_json);
LA_ADSC_FORMATTER_FUN(la_adsc_intermediate_projection_format_json);
LA_ADSC_FORMATTER_FUN(la_adsc_fixed_projection_format_json);
LA_ADSC_FORMATTER_FUN(la_adsc_meteo_format_json);
LA_ADSC_FORMATTER_FUN(la_adsc_airframe_id_format_json);

static void la_adsc_contract_request_destroy(void *data);
static void la_adsc_noncomp_notify_destroy(void *data);

/*****************
 * Downlink tags
 *****************/

static la_dict const la_adsc_downlink_tag_descriptor_table[] = {
	{
		.id = 3,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Acknowledgement",
			.json_key = "ack",
			.parse = la_adsc_uint8_t_parse,
			.format_text = la_adsc_tag_with_contract_number_format_text,
			.format_json = la_adsc_tag_with_contract_number_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 4,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Negative acknowledgement",
			.json_key = "nack",
			.parse = la_adsc_nack_parse,
			.format_text = la_adsc_nack_format_text,
			.format_json = la_adsc_nack_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 5,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Noncompliance notification",
			.json_key = "noncomp_notify",
			.parse = la_adsc_noncomp_notify_parse,
			.format_text = la_adsc_noncomp_notify_format_text,
			.format_json = la_adsc_noncomp_notify_format_json,
			.destroy = la_adsc_noncomp_notify_destroy
		}
	},
	{
		.id = 6,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Cancel emergency mode",
			.json_key = "cancel_emergency_mode",
			.parse = NULL,
			.format_text = la_adsc_empty_tag_format_text,
			.format_json = la_adsc_empty_tag_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 7,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Basic report",
			.json_key = "basic_report",
			.parse = la_adsc_basic_report_parse,
			.format_text = la_adsc_basic_report_format_text,
			.format_json = la_adsc_basic_report_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 9,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Emergency basic report",
			.json_key = "emerg_basic_report",
			.parse = la_adsc_basic_report_parse,
			.format_text = la_adsc_basic_report_format_text,
			.format_json = la_adsc_basic_report_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 10,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Lateral deviation change event",
			.json_key = "lat_dev_change_event",
			.parse = la_adsc_basic_report_parse,
			.format_text = la_adsc_basic_report_format_text,
			.format_json = la_adsc_basic_report_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 12,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Flight ID data",
			.json_key = "flight_id",
			.parse = la_adsc_flight_id_parse,
			.format_text = la_adsc_flight_id_format_text,
			.format_json = la_adsc_flight_id_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 13,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Predicted route",
			.json_key = "predicted_route",
			.parse = la_adsc_predicted_route_parse,
			.format_text = la_adsc_predicted_route_format_text,
			.format_json = la_adsc_predicted_route_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 14,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Earth reference data",
			.json_key = "earth_ref_data",
			.parse = la_adsc_earth_air_ref_parse,
			.format_text = la_adsc_earth_ref_format_text,
			.format_json = la_adsc_earth_ref_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 15,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Air reference data",
			.json_key = "air_ref_data",
			.parse = la_adsc_earth_air_ref_parse,
			.format_text = la_adsc_air_ref_format_text,
			.format_json = la_adsc_air_ref_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 16,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Meteo data",
			.json_key = "meteo_data",
			.parse = la_adsc_meteo_parse,
			.format_text = la_adsc_meteo_format_text,
			.format_json = la_adsc_meteo_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 17,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Airframe ID",
			.json_key = "airframe_id",
			.parse = la_adsc_airframe_id_parse,
			.format_text = la_adsc_airframe_id_format_text,
			.format_json = la_adsc_airframe_id_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 18,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Vertical rate change event",
			.json_key = "vspd_chg_event",
			.parse = la_adsc_basic_report_parse,
			.format_text = la_adsc_basic_report_format_text,
			.format_json = la_adsc_basic_report_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 19,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Altitude range event",
			.json_key = "alt_range_event",
			.parse = la_adsc_basic_report_parse,
			.format_text = la_adsc_basic_report_format_text,
			.format_json = la_adsc_basic_report_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 20,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Waypoint change event",
			.json_key = "wpt_change_event",
			.parse = la_adsc_basic_report_parse,
			.format_text = la_adsc_basic_report_format_text,
			.format_json = la_adsc_basic_report_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 22,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Intermediate projection",
			.json_key = "intermediate_projection",
			.parse = la_adsc_intermediate_projection_parse,
			.format_text = la_adsc_intermediate_projection_format_text,
			.format_json = la_adsc_intermediate_projection_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 23,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Fixed projection",
			.json_key = "fixed_projection",
			.parse = la_adsc_fixed_projection_parse,
			.format_text = la_adsc_fixed_projection_format_text,
			.format_json = la_adsc_fixed_projection_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 255,      // Fake tag for reason code in DIS message
		.val = &(la_adsc_type_descriptor_t){
			.label = "Reason",
			.json_key = "reason",
			.parse = la_adsc_uint8_t_parse,
			.format_text = la_adsc_dis_reason_code_format_text,
			.format_json = la_adsc_dis_reason_code_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 0,
		.val = NULL
	}
};

/**************************
 * Downlink tag destructors
 **************************/

static void la_adsc_noncomp_notify_destroy(void *data) {
	if(data == NULL)
		return;
	la_adsc_noncomp_notify_t *n = data;
	LA_XFREE(n->groups);
	LA_XFREE(data);
	return;
}

/**********************
 * Downlink tag parsers
 **********************/

#define LA_BS_READ_OR_RETURN(bs, dest, len, ret) \
	if(la_bitstream_read_word_msbfirst(bs, dest, len) < 0) { return ret; }

LA_ADSC_PARSER_FUN(la_adsc_nack_parse) {
	uint32_t tag_len = 2;
	la_adsc_tag_t *t = dest;
	LA_ADSC_CHECK_LEN(t->tag, len, tag_len);
	LA_NEW(la_adsc_nack_t, n);

	n->contract_req_num = buf[0];
	if(buf[1] > LA_ADSC_NACK_MAX_REASON_CODE) {
		la_debug_print(D_ERROR, "Invalid reason code: %u\n", buf[1]);
		goto fail;
	}
	n->reason = buf[1];
	la_debug_print(D_INFO, "reason: %u\n", n->reason);

	// these reason codes have extended data byte
	if(buf[1] == 1 || buf[1] == 2 || buf[1] == 7) {
		tag_len++;
		if(len < tag_len) {
			la_debug_print(D_ERROR, "Truncated tag %u: len: %u < %u\n", t->tag, len, tag_len);
			goto fail;
		}
		n->ext_data = buf[2];
		la_debug_print(D_INFO, "ext_data: %u\n", n->ext_data);
	}
	t->data = n;
	return tag_len;
fail:
	LA_XFREE(n);
	return -1;
}

LA_ADSC_PARSER_FUN(la_adsc_noncomp_group_parse) {
	uint32_t tag_len = 2;
	la_adsc_noncomp_group_t *g = dest;
	if(len < tag_len) {
		la_debug_print(D_ERROR, "too short: %u < %u\n", len, tag_len);
		return -1;
	}

	g->noncomp_tag = buf[0];
	g->is_unrecognized = (buf[1] & 0x80) ? 1 : 0;
	g->is_whole_group_unavail = (buf[1] & 0x40) ? 1 : 0;
	la_debug_print(D_INFO, "tag: %u unrecognized: %u whole_group: %u\n",
			g->noncomp_tag, g->is_unrecognized, g->is_whole_group_unavail);

	if(g->is_unrecognized || g->is_whole_group_unavail) {
		return tag_len;
	}
	g->param_cnt = buf[1] & 0xf;
	la_debug_print(D_INFO, "param_cnt: %u\n", g->param_cnt);
	if(g->param_cnt == 0) {
		return tag_len;
	}

	// following octets contain 4-bit numbers of non-compliant parameters (up to 15)
	tag_len += g->param_cnt / 2 + g->param_cnt % 2;
	la_debug_print(D_INFO, "new tag_len: %u\n", tag_len);
	if(len < tag_len) {
		la_debug_print(D_ERROR, "too short: %u < %u\n", len, tag_len);
		return -1;
	}
	buf += 2; len -= 2;
	for(int i = 0; i < g->param_cnt; i++) {
		// store nibbles separately
		g->params[i] = (*buf >> (((i + 1) % 2) * 4)) & 0xf;
		buf += i % 2; len -= i % 2;
	}
	return tag_len;
}

LA_ADSC_PARSER_FUN(la_adsc_noncomp_notify_parse) {
	uint32_t tag_len = 2;
	la_adsc_tag_t *t = dest;
	LA_ADSC_CHECK_LEN(t->tag, len, tag_len);
	LA_NEW(la_adsc_noncomp_notify_t, n);
	t->data = n;

	n->contract_req_num = buf[0];
	n->group_cnt = buf[1];
	if(n->group_cnt == 0) {
		return tag_len;
	}
	la_debug_print(D_INFO, "group_cnt: %u\n", n->group_cnt);
	n->groups = LA_XCALLOC(n->group_cnt, sizeof(la_adsc_noncomp_group_t));
	buf += 2; len -= 2;
	int consumed_bytes = 0;
	for(uint8_t i = 0; i < n->group_cnt; i++) {
		la_debug_print(D_INFO, "Remaining length: %u\n", len);
		if((consumed_bytes = la_adsc_noncomp_group_parse(n->groups + i, buf, len)) < 0) {
			return -1;
		}
		buf += consumed_bytes; len -= consumed_bytes;
		tag_len += consumed_bytes;
		if(len == 0) {
			if(i < n->group_cnt - 1) {
				la_debug_print(D_ERROR, "truncated: read %u/%u groups\n", i + 1, n->group_cnt);
				return -1;
			} else {
				break;      // parsing completed
			}
		}
	}
	return tag_len;
}

LA_ADSC_PARSER_FUN(la_adsc_basic_report_parse) {
	uint32_t tag_len = 10;
	la_adsc_tag_t *t = dest;
	LA_ADSC_CHECK_LEN(t->tag, len, tag_len);
	LA_NEW(la_adsc_basic_report_t, r);
	t->data = r;

	la_bitstream_t *bs = la_bitstream_init(tag_len * 8);
	if(la_bitstream_append_msbfirst(bs, buf, tag_len, 8) < 0) {
		return -1;
	}

	uint32_t tmp;
	LA_BS_READ_OR_RETURN(bs, &tmp, 21, -1);
	r->lat = la_adsc_coordinate_parse(tmp);
	LA_BS_READ_OR_RETURN(bs, &tmp, 21, -1);
	r->lon = la_adsc_coordinate_parse(tmp);
	LA_BS_READ_OR_RETURN(bs, &tmp, 16, -1);
	r->alt = la_adsc_altitude_parse(tmp);
	LA_BS_READ_OR_RETURN(bs, &tmp, 15, -1);
	r->timestamp = la_adsc_timestamp_parse(tmp);
	LA_BS_READ_OR_RETURN(bs, &tmp, 7, -1);
	r->redundancy = (uint8_t)(tmp & 1);
	r->accuracy = (uint8_t)((tmp >> 1) & 0x7);
	r->tcas_health = (uint8_t)((tmp >> 4) & 1);
	la_debug_print(D_INFO, "redundancy: %u accuracy: %u TCAS: %u\n",
			r->redundancy, r->accuracy, r->tcas_health);

	la_bitstream_destroy(bs);
	return tag_len;
}

LA_ADSC_PARSER_FUN(la_adsc_flight_id_parse) {
	uint32_t tag_len = 6;
	la_adsc_tag_t *t = dest;
	LA_ADSC_CHECK_LEN(t->tag, len, tag_len);
	LA_NEW(la_adsc_flight_id_t, f);
	t->data = f;

	la_bitstream_t *bs = la_bitstream_init(tag_len * 8);
	if(la_bitstream_append_msbfirst(bs, buf, tag_len, 8) < 0) {
		return -1;
	}
	uint32_t tmp = 0;
	for(size_t i = 0; i < sizeof(f->id) - 1; i++) {
		// ISO5 alphabet on 6 bits, valid characters: A-Z, 0-9, space
		// (00) 10 0000 - space
		// (01) 0x xxxx - A-Z
		// (00) 11 xxxx - 0-9
		LA_BS_READ_OR_RETURN(bs, &tmp, 6, -1);
		if((tmp & 0x20) == 0)
			tmp += 0x40;
		f->id[i] = (uint8_t)tmp;
	}
	f->id[sizeof(f->id) - 1] = '\0';
	la_debug_print(D_INFO, "%s\n", f->id);
	la_bitstream_destroy(bs);
	return tag_len;
}

LA_ADSC_PARSER_FUN(la_adsc_predicted_route_parse) {
	uint32_t tag_len = 17;
	la_adsc_tag_t *t = dest;
	LA_ADSC_CHECK_LEN(t->tag, len, tag_len);
	LA_NEW(la_adsc_predicted_route_t, r);
	t->data = r;

	la_bitstream_t *bs = la_bitstream_init(tag_len * 8);
	if(la_bitstream_append_msbfirst(bs, buf, tag_len, 8) < 0) {
		return -1;
	}

	uint32_t tmp;
	LA_BS_READ_OR_RETURN(bs, &tmp, 21, -1);
	r->lat_next = la_adsc_coordinate_parse(tmp);
	LA_BS_READ_OR_RETURN(bs, &tmp, 21, -1);
	r->lon_next = la_adsc_coordinate_parse(tmp);
	LA_BS_READ_OR_RETURN(bs, &tmp, 16, -1);
	r->alt_next = la_adsc_altitude_parse(tmp);
	LA_BS_READ_OR_RETURN(bs, &tmp, 14, -1);
	r->eta_next = tmp;
	la_debug_print(D_INFO, "eta: %d\n", r->eta_next);
	LA_BS_READ_OR_RETURN(bs, &tmp, 21, -1);
	r->lat_next_next = la_adsc_coordinate_parse(tmp);
	LA_BS_READ_OR_RETURN(bs, &tmp, 21, -1);
	r->lon_next_next = la_adsc_coordinate_parse(tmp);
	LA_BS_READ_OR_RETURN(bs, &tmp, 16, -1);
	r->alt_next_next = la_adsc_altitude_parse(tmp);

	la_bitstream_destroy(bs);
	return tag_len;
}

LA_ADSC_PARSER_FUN(la_adsc_earth_air_ref_parse) {
	uint32_t tag_len = 5;
	la_adsc_tag_t *t = dest;
	LA_ADSC_CHECK_LEN(t->tag, len, tag_len);
	LA_NEW(la_adsc_earth_air_ref_t, r);
	t->data = r;

	la_bitstream_t *bs = la_bitstream_init(tag_len * 8);
	if(la_bitstream_append_msbfirst(bs, buf, tag_len, 8) < 0) {
		return -1;
	}

	uint32_t tmp;
	LA_BS_READ_OR_RETURN(bs, &tmp, 1, -1);
	r->heading_invalid = tmp;
	LA_BS_READ_OR_RETURN(bs, &tmp, 12, -1);
	r->heading = la_adsc_heading_parse(tmp);
	LA_BS_READ_OR_RETURN(bs, &tmp, 13, -1);
	r->speed = la_adsc_speed_parse(tmp);
	LA_BS_READ_OR_RETURN(bs, &tmp, 12, -1);
	r->vert_speed = la_adsc_vert_speed_parse(tmp);

	la_bitstream_destroy(bs);
	return tag_len;
}

LA_ADSC_PARSER_FUN(la_adsc_intermediate_projection_parse) {
	uint32_t tag_len = 8;
	la_adsc_tag_t *t = dest;
	LA_ADSC_CHECK_LEN(t->tag, len, tag_len);
	LA_NEW(la_adsc_intermediate_projection_t, p);
	t->data = p;

	la_bitstream_t *bs = la_bitstream_init(tag_len * 8);
	if(la_bitstream_append_msbfirst(bs, buf, tag_len, 8) < 0) {
		return -1;
	}

	uint32_t tmp;
	LA_BS_READ_OR_RETURN(bs, &tmp, 16, -1);
	p->distance = la_adsc_distance_parse(tmp);
	LA_BS_READ_OR_RETURN(bs, &tmp, 1, -1);
	p->track_invalid = tmp;
	LA_BS_READ_OR_RETURN(bs, &tmp, 12, -1);
	p->track = la_adsc_heading_parse(tmp);
	LA_BS_READ_OR_RETURN(bs, &tmp, 16, -1);
	p->alt = la_adsc_altitude_parse(tmp);
	LA_BS_READ_OR_RETURN(bs, &tmp, 14, -1);
	p->eta = tmp;
	la_debug_print(D_INFO, "eta: %d\n", p->eta);

	la_bitstream_destroy(bs);
	return tag_len;
}

LA_ADSC_PARSER_FUN(la_adsc_fixed_projection_parse) {
	uint32_t tag_len = 9;
	la_adsc_tag_t *t = dest;
	LA_ADSC_CHECK_LEN(t->tag, len, tag_len);
	LA_NEW(la_adsc_fixed_projection_t, p);
	t->data = p;

	la_bitstream_t *bs = la_bitstream_init(tag_len * 8);
	if(la_bitstream_append_msbfirst(bs, buf, tag_len, 8) < 0) {
		return -1;
	}

	uint32_t tmp;
	LA_BS_READ_OR_RETURN(bs, &tmp, 21, -1);
	p->lat = la_adsc_coordinate_parse(tmp);
	LA_BS_READ_OR_RETURN(bs, &tmp, 21, -1);
	p->lon = la_adsc_coordinate_parse(tmp);
	LA_BS_READ_OR_RETURN(bs, &tmp, 16, -1);
	p->alt = la_adsc_altitude_parse(tmp);
	LA_BS_READ_OR_RETURN(bs, &tmp, 14, -1);
	p->eta = tmp;
	la_debug_print(D_INFO, "eta: %d\n", p->eta);

	la_bitstream_destroy(bs);
	return tag_len;
}

LA_ADSC_PARSER_FUN(la_adsc_meteo_parse) {
	uint32_t tag_len = 4;
	la_adsc_tag_t *t = dest;
	LA_ADSC_CHECK_LEN(t->tag, len, tag_len);
	LA_NEW(la_adsc_meteo_t, m);
	t->data = m;

	la_bitstream_t *bs = la_bitstream_init(tag_len * 8);
	if(la_bitstream_append_msbfirst(bs, buf, tag_len, 8) < 0) {
		return -1;
	}

	uint32_t tmp;
	LA_BS_READ_OR_RETURN(bs, &tmp, 9, -1);
	m->wind_speed = la_adsc_speed_parse(tmp);
	LA_BS_READ_OR_RETURN(bs, &tmp, 1, -1);
	m->wind_dir_invalid = tmp;
	LA_BS_READ_OR_RETURN(bs, &tmp, 9, -1);
	m->wind_dir = la_adsc_wind_dir_parse(tmp);
	LA_BS_READ_OR_RETURN(bs, &tmp, 12, -1);
	m->temp = la_adsc_temperature_parse(tmp);

	la_bitstream_destroy(bs);
	return tag_len;
}

LA_ADSC_PARSER_FUN(la_adsc_airframe_id_parse) {
	uint32_t tag_len = 3;
	la_adsc_tag_t *t = dest;
	LA_ADSC_CHECK_LEN(t->tag, len, tag_len);
	LA_NEW(la_adsc_airframe_id_t, a);
	t->data = a;

	memcpy(a->icao_hex, buf, tag_len);
	return tag_len;
}

/*************************
 * Downlink tag formatters
 *************************/

LA_ADSC_FORMATTER_FUN(la_adsc_nack_format_text) {
	static char const *reason_code_table[LA_ADSC_NACK_MAX_REASON_CODE+1] = {
		[0] = NULL,
		[1] = "Duplicate group tag",
		[2] = "Duplicate reporting interval tag",
		[3] = "Event contract request with no data",
		[4] = "Improper operational mode tag",
		[5] = "Cancel request of a contract which does not exist",
		[6] = "Requested contract already exists",
		[7] = "Undefined contract request tag",
		[8] = "Undefined error",
		[9] = "Not enough data in request",
		[10] = "Invalid altitude range: low limit >= high limit",
		[11] = "Vertical speed threshold is 0",
		[12] = "Aircraft intent projection time is 0",
		[13] = "Lateral deviation threshold is 0"
	};
	la_adsc_nack_t const *n = data;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "%s:\n", label);
	ctx->indent++;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Contract request number: %u\n", n->contract_req_num);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Reason: %u (%s)\n", n->reason, reason_code_table[n->reason]);
	if(n->reason == 1 || n->reason == 2) {
		LA_ISPRINTF(ctx->vstr, ctx->indent, "Erroneous octet number: %u\n", n->ext_data);
	} else if(n->reason == 7) {
		LA_ISPRINTF(ctx->vstr, ctx->indent, "Erroneous tag number: %u\n", n->ext_data);
	}
	ctx->indent--;
}

LA_ADSC_FORMATTER_FUN(la_adsc_nack_format_json) {
	LA_UNUSED(label);
	la_adsc_nack_t const *n = data;
	la_json_append_int64(ctx->vstr, "contract_req_num", n->contract_req_num);
	la_json_append_int64(ctx->vstr, "reason", n->reason);
	if(n->reason == 1 || n->reason == 2) {
		la_json_append_int64(ctx->vstr, "err_octet", n->ext_data);
	} else if(n->reason == 7) {
		la_json_append_int64(ctx->vstr, "err_tag", n->ext_data);
	}
}

LA_ADSC_FORMATTER_FUN(la_adsc_dis_reason_code_format_text) {
	static la_dict const dis_reason_code_table[] = {
		{ 0, "reason not specified" },
		{ 1, "congestion" },
		{ 2, "application not available" },
		{ 8, "normal disconnect" },
		{ 0, NULL }
	};
	uint8_t const *rc = data;
	int reason = (int)(*rc >> 4);
	char *descr = la_dict_search(dis_reason_code_table, reason);
	if(descr) {
		LA_ISPRINTF(ctx->vstr, ctx->indent, "%s: %s\n", label, descr);
	} else {
		LA_ISPRINTF(ctx->vstr, ctx->indent, "%s: unknown (%d)\n", label, reason);
	}
}

LA_ADSC_FORMATTER_FUN(la_adsc_dis_reason_code_format_json) {
	LA_UNUSED(label);
	uint8_t const *rc = data;
	int reason = (int)(*rc >> 4);
	la_json_append_int64(ctx->vstr, "reason_code", reason);
}

LA_ADSC_FORMATTER_FUN(la_adsc_noncomp_group_format_text) {
	LA_UNUSED(label);
	la_adsc_noncomp_group_t const *g = data;

	LA_ISPRINTF(ctx->vstr, ctx->indent, "Tag %u:\n", g->noncomp_tag);
	ctx->indent++;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "%s",
			g->is_unrecognized ? "Unrecognized group" :
			(g->is_whole_group_unavail ? "Unavailable group" : "Unavailable parameters: ")
			);
	if(!g->is_unrecognized && !g->is_whole_group_unavail && g->param_cnt > 0) {
		for(int i = 0; i < g->param_cnt; i++) {
			la_vstring_append_sprintf(ctx->vstr, "%d ", g->params[i]);
		}
	}
	la_vstring_append_sprintf(ctx->vstr, "\n");
	ctx->indent--;
}

LA_ADSC_FORMATTER_FUN(la_adsc_noncomp_group_format_json) {
	LA_UNUSED(label);
	la_adsc_noncomp_group_t const *g = data;

	la_json_append_int64(ctx->vstr, "noncomp_tag", g->noncomp_tag);
	la_json_append_string(ctx->vstr, "noncomp_cause",
			g->is_unrecognized ? "group_unrecognized" :
			(g->is_whole_group_unavail ? "group_unavailable" : "params_unavailable"));
	if(!g->is_unrecognized && !g->is_whole_group_unavail) {
		la_json_array_start(ctx->vstr, "params");
		if(g->param_cnt > 0) {
			for(int i = 0; i < g->param_cnt; i++) {
				la_json_append_int64(ctx->vstr, NULL, g->params[i]);
			}
		}
		la_json_array_end(ctx->vstr);
	}
}

LA_ADSC_FORMATTER_FUN(la_adsc_noncomp_notify_format_text) {
	la_adsc_noncomp_notify_t const *n = data;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "%s:\n", label);
	ctx->indent++;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Contract number: %u\n", n->contract_req_num);
	if(n->group_cnt > 0) {
		for(int i = 0; i < n->group_cnt; i++) {
			la_adsc_noncomp_group_format_text(ctx, NULL, n->groups + i);
		}
	}
	ctx->indent--;
}

LA_ADSC_FORMATTER_FUN(la_adsc_noncomp_notify_format_json) {
	LA_UNUSED(label);
	la_adsc_noncomp_notify_t const *n = data;
	la_json_append_int64(ctx->vstr, "contract_req_num", n->contract_req_num);
	la_json_array_start(ctx->vstr, "msg_groups");
	if(n->group_cnt > 0) {
		for(int i = 0; i < n->group_cnt; i++) {
			la_json_object_start(ctx->vstr, NULL);
			la_adsc_noncomp_group_format_json(ctx, NULL, n->groups + i);
			la_json_object_end(ctx->vstr);
		}
	}
	la_json_array_end(ctx->vstr);
}

LA_ADSC_FORMATTER_FUN(la_adsc_basic_report_format_text) {
	static char const *accuracy_table[] = {
		[0] = "none (NAV capability lost)",
		[1] = "<30 nm",
		[2] = "<15 nm",
		[3] = "<8 nm",
		[4] = "<4 nm",
		[5] = "<1 nm",
		[6] = "<0.25 nm",
		[7] = "<0.05 nm"
	};
	static char const *redundancy_state_table[] = {
		[0] = "lost",
		[1] = "OK"
	};
	static char const *tcas_state_table[] = {
		[0] = "not available to ADS",
		[1] = "OK"
	};
	la_adsc_basic_report_t const *r = data;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "%s:\n", label);
	ctx->indent++;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Lat: %.7f\n", r->lat);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Lon: %.7f\n", r->lon);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Alt: %d ft\n", r->alt);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Time: %.3f sec past hour (:%02.0f:%06.3f)\n",
			r->timestamp,
			trunc(r->timestamp / 60.0),
			r->timestamp - 60.0 * trunc(r->timestamp / 60.0)
			);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Position accuracy: %s\n", accuracy_table[r->accuracy]);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "NAV unit redundancy: %s\n", redundancy_state_table[r->redundancy]);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "TCAS: %s\n", tcas_state_table[r->tcas_health]);
	ctx->indent--;
}

LA_ADSC_FORMATTER_FUN(la_adsc_basic_report_format_json) {
	LA_UNUSED(label);
	static float const accuracy_table[] = {
		[0] = -1.0f,    // NAV capability lost
		[1] = 30.0f,    // less than 30 nm
		[2] = 15.0f,    // ... etc.
		[3] = 8.0f,
		[4] = 4.0f,
		[5] = 1.0f,
		[6] = 0.25f,
		[7] = 0.05f
	};
	static bool const redundancy_state_table[] = {
		[0] = false,
		[1] = true
	};
	static char const tcas_state_table[] = {
		[0] = false,
		[1] = true
	};
	la_adsc_basic_report_t const *r = data;

	la_json_append_double(ctx->vstr, "lat", r->lat);
	la_json_append_double(ctx->vstr, "lon", r->lon);
	la_json_append_int64(ctx->vstr, "alt", r->alt);
	la_json_append_double(ctx->vstr, "ts_sec", r->timestamp);
	la_json_append_double(ctx->vstr, "pos_accuracy_nm", accuracy_table[r->accuracy]);
	la_json_append_bool(ctx->vstr, "nav_redundancy", redundancy_state_table[r->redundancy]);
	la_json_append_bool(ctx->vstr, "tcas_avail", tcas_state_table[r->tcas_health]);
}

LA_ADSC_FORMATTER_FUN(la_adsc_flight_id_format_text) {
	la_adsc_flight_id_t const *f = data;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "%s:\n", label);
	ctx->indent++;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Flight ID: %s\n", f->id);
	ctx->indent--;
}

LA_ADSC_FORMATTER_FUN(la_adsc_flight_id_format_json) {
	LA_UNUSED(label);
	la_adsc_flight_id_t const *f = data;
	la_json_append_string(ctx->vstr, "flight_id", f->id);
}

LA_ADSC_FORMATTER_FUN(la_adsc_predicted_route_format_text) {
	la_adsc_predicted_route_t const *r = data;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "%s:\n", label);
	ctx->indent++;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Next waypoint:\n");
	ctx->indent++;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Lat: %.7f\n", r->lat_next);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Lon: %.7f\n", r->lon_next);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Alt: %d ft\n", r->alt_next);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "ETA: %d sec\n", r->eta_next);
	ctx->indent--;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Next+1 waypoint:\n");
	ctx->indent++;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Lat: %.7f\n", r->lat_next_next);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Lon: %.7f\n", r->lon_next_next);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Alt: %d ft\n", r->alt_next_next);
	ctx->indent--;
	ctx->indent--;
}

LA_ADSC_FORMATTER_FUN(la_adsc_predicted_route_format_json) {
	LA_UNUSED(label);
	la_adsc_predicted_route_t const *r = data;
	la_json_object_start(ctx->vstr, "next_wpt");
	la_json_append_double(ctx->vstr, "lat", r->lat_next);
	la_json_append_double(ctx->vstr, "lon", r->lon_next);
	la_json_append_int64(ctx->vstr, "alt", r->alt_next);
	la_json_append_int64(ctx->vstr, "eta_sec", r->eta_next);
	la_json_object_end(ctx->vstr);
	la_json_object_start(ctx->vstr, "next_next_wpt");
	la_json_append_double(ctx->vstr, "lat", r->lat_next_next);
	la_json_append_double(ctx->vstr, "lon", r->lon_next_next);
	la_json_append_int64(ctx->vstr, "alt", r->alt_next_next);
	la_json_object_end(ctx->vstr);
}

LA_ADSC_FORMATTER_FUN(la_adsc_earth_ref_format_text) {
	la_adsc_earth_air_ref_t const *r = data;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "%s:\n", label);
	ctx->indent++;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "True track: %.1f deg%s\n", r->heading, r->heading_invalid ? " (invalid)" : "");
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Ground speed: %.1f kt\n", r->speed);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Vertical speed: %d ft/min\n", r->vert_speed);
	ctx->indent--;
}

LA_ADSC_FORMATTER_FUN(la_adsc_earth_ref_format_json) {
	LA_UNUSED(label);
	la_adsc_earth_air_ref_t const *r = data;
	la_json_append_double(ctx->vstr, "true_trk_deg", r->heading);
	la_json_append_bool(ctx->vstr, "true_trk_valid", r->heading_invalid ? false : true);
	la_json_append_double(ctx->vstr, "gnd_spd_kts", r->speed);
	la_json_append_int64(ctx->vstr, "vspd_ftmin", r->vert_speed);
}

LA_ADSC_FORMATTER_FUN(la_adsc_air_ref_format_text) {
	la_adsc_earth_air_ref_t const *r = data;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "%s:\n", label);
	ctx->indent++;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "True heading: %.1f deg%s\n", r->heading, r->heading_invalid ? " (invalid)" : "");
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Mach speed: %.4f\n", r->speed / 1000.0);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Vertical speed: %d ft/min\n", r->vert_speed);
	ctx->indent--;
}

LA_ADSC_FORMATTER_FUN(la_adsc_air_ref_format_json) {
	LA_UNUSED(label);
	la_adsc_earth_air_ref_t const *r = data;
	la_json_append_double(ctx->vstr, "true_hdg_deg", r->heading);
	la_json_append_bool(ctx->vstr, "true_hdg_valid", r->heading_invalid ? false : true);
	la_json_append_double(ctx->vstr, "spd_mach", r->speed / 1000.0);
	la_json_append_double(ctx->vstr, "vspd_ftmin", r->vert_speed);
}

LA_ADSC_FORMATTER_FUN(la_adsc_intermediate_projection_format_text) {
	la_adsc_intermediate_projection_t const *p = data;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "%s:\n", label);
	ctx->indent++;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Distance: %.3f nm\n", p->distance);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "True track: %.1f deg%s\n", p->track, p->track_invalid ? " (invalid)" : "");
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Alt: %d ft\n", p->alt);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "ETA: %d sec\n", p->eta);
	ctx->indent--;
}

LA_ADSC_FORMATTER_FUN(la_adsc_intermediate_projection_format_json) {
	LA_UNUSED(label);
	la_adsc_intermediate_projection_t const *p = data;
	la_json_append_double(ctx->vstr, "dist_nm", p->distance);
	la_json_append_double(ctx->vstr, "true_trk_deg", p->track);
	la_json_append_bool(ctx->vstr, "true_trk_valid", p->track_invalid ? false : true);
	la_json_append_int64(ctx->vstr, "alt", p->alt);
	la_json_append_int64(ctx->vstr, "eta_sec", p->eta);
}

LA_ADSC_FORMATTER_FUN(la_adsc_fixed_projection_format_text) {
	la_adsc_fixed_projection_t const *p = data;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "%s:\n", label);
	ctx->indent++;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Lat: %.7f\n", p->lat);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Lon: %.7f\n", p->lon);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Alt: %d ft\n", p->alt);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "ETA: %d sec\n", p->eta);
	ctx->indent--;
}

LA_ADSC_FORMATTER_FUN(la_adsc_fixed_projection_format_json) {
	LA_UNUSED(label);
	la_adsc_fixed_projection_t const *p = data;
	la_json_append_double(ctx->vstr, "lat", p->lat);
	la_json_append_double(ctx->vstr, "lon", p->lon);
	la_json_append_int64(ctx->vstr, "alt", p->alt);
	la_json_append_int64(ctx->vstr, "eta_sec", p->eta);
}

LA_ADSC_FORMATTER_FUN(la_adsc_meteo_format_text) {
	la_adsc_meteo_t const *m = data;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "%s:\n", label);
	ctx->indent++;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Wind speed: %.1f kt\n", m->wind_speed);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "True wind direction: %.1f deg%s\n", m->wind_dir, m->wind_dir_invalid ? " (invalid)" : "");
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Temperature: %.2f C\n", m->temp);
	ctx->indent--;
}

LA_ADSC_FORMATTER_FUN(la_adsc_meteo_format_json) {
	LA_UNUSED(label);
	la_adsc_meteo_t const *m = data;
	la_json_append_double(ctx->vstr, "wind_spd_kts", m->wind_speed);
	la_json_append_double(ctx->vstr, "wind_dir_true_deg", m->wind_dir);
	la_json_append_bool(ctx->vstr, "wind_dir_valid", m->wind_dir_invalid ? false : true);
	la_json_append_double(ctx->vstr, "temp_c", m->temp);
}

LA_ADSC_FORMATTER_FUN(la_adsc_airframe_id_format_text) {
	la_adsc_airframe_id_t const *a = data;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "%s:\n", label);
	ctx->indent++;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "ICAO ID: %02X%02X%02X\n", a->icao_hex[0], a->icao_hex[1], a->icao_hex[2]);
	ctx->indent--;
}

LA_ADSC_FORMATTER_FUN(la_adsc_airframe_id_format_json) {
	LA_UNUSED(label);
	la_adsc_airframe_id_t const *a = data;
	la_json_append_int64(ctx->vstr, "icao_id",
			((long)(a->icao_hex[0]) << 16) |
			((long)(a->icao_hex[1]) << 8)  |
			(long)(a->icao_hex[2])
			);
}

/****************
 * Uplink tags
 ****************/

static la_dict const la_adsc_uplink_tag_descriptor_table[] = {
	{
		.id = 1,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Cancel all contracts and terminate connection",
			.json_key = "cancel_all_contracts",
			.parse = NULL,
			.format_text = la_adsc_empty_tag_format_text,
			.format_json = la_adsc_empty_tag_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 2,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Cancel contract",
			.json_key = "cancel_contract",
			.parse = la_adsc_uint8_t_parse,
			.format_text = la_adsc_tag_with_contract_number_format_text,
			.format_json = la_adsc_tag_with_contract_number_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 6,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Cancel emergency mode",
			.json_key = "cancel_emergency_mode",
			.parse = la_adsc_uint8_t_parse,
			.format_text = la_adsc_tag_with_contract_number_format_text,
			.format_json = la_adsc_tag_with_contract_number_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 7,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Periodic contract request",
			.json_key = "periodic_contract_req",
			.parse = la_adsc_contract_request_parse,
			.format_text = la_adsc_contract_request_format_text,
			.format_json = la_adsc_contract_request_format_json,
			.destroy = la_adsc_contract_request_destroy
		}
	},
	{
		.id = 8,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Event contract request",
			.json_key = "event_contract_req",
			.parse = la_adsc_contract_request_parse,
			.format_text = la_adsc_contract_request_format_text,
			.format_json = la_adsc_contract_request_format_json,
			.destroy = la_adsc_contract_request_destroy
		}
	},
	{
		.id = 9,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Emergency periodic contract request",
			.json_key = "emerg_periodic_contract_req",
			.parse = la_adsc_contract_request_parse,
			.format_text = la_adsc_contract_request_format_text,
			.format_json = la_adsc_contract_request_format_json,
			.destroy = la_adsc_contract_request_destroy
		}
	},
	{
		.id = 0,
		.val = NULL
	}
};

static la_dict const la_adsc_request_tag_descriptor_table[] = {
	{
		.id = 10,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Report when lateral deviation exceeds",
			.json_key = "report_when_lateral_dev_exceeds",
			.parse = la_adsc_lat_dev_change_parse,
			.format_text = la_adsc_lat_dev_change_format_text,
			.format_json = la_adsc_lat_dev_change_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 11,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Reporting interval",
			.json_key = "report_interval",
			.parse = la_adsc_reporting_interval_parse,
			.format_text = la_adsc_reporting_interval_format_text,
			.format_json = la_adsc_reporting_interval_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 12,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Flight ID",
			.json_key = "flight_id",
			.parse = la_adsc_uint8_t_parse,
			.format_text = la_adsc_modulus_format_text,
			.format_json = la_adsc_modulus_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 13,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Predicted route",
			.json_key = "predicted_route",
			.parse = la_adsc_uint8_t_parse,
			.format_text = la_adsc_modulus_format_text,
			.format_json = la_adsc_modulus_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 14,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Earth reference data",
			.json_key = "earth_ref_data",
			.parse = la_adsc_uint8_t_parse,
			.format_text = la_adsc_modulus_format_text,
			.format_json = la_adsc_modulus_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 15,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Air reference data",
			.json_key = "air_ref_data",
			.parse = la_adsc_uint8_t_parse,
			.format_text = la_adsc_modulus_format_text,
			.format_json = la_adsc_modulus_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 16,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Meteo data",
			.json_key = "meteo_data",
			.parse = la_adsc_uint8_t_parse,
			.format_text = la_adsc_modulus_format_text,
			.format_json = la_adsc_modulus_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 17,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Airframe ID",
			.json_key = "airframe_id",
			.parse = la_adsc_uint8_t_parse,
			.format_text = la_adsc_modulus_format_text,
			.format_json = la_adsc_modulus_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 18,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Report when vertical speed is",
			.json_key = "report_when_vspd_is",
			.parse = la_adsc_vspd_change_parse,
			.format_text = la_adsc_vspd_change_format_text,
			.format_json = la_adsc_vspd_change_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 19,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Report when altitude out of range",
			.json_key = "report_when_alt_out_of_range",
			.parse = la_adsc_alt_range_parse,
			.format_text = la_adsc_alt_range_format_text,
			.format_json = la_adsc_alt_range_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 20,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Report waypoint changes",
			.json_key = "report_wpt_changes",
			.parse = NULL,
			.format_text = la_adsc_empty_tag_format_text,
			.format_json = la_adsc_empty_tag_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 21,
		.val = &(la_adsc_type_descriptor_t){
			.label = "Aircraft intent data",
			.json_key = "acft_intent_data",
			.parse = la_adsc_acft_intent_group_parse,
			.format_text = la_adsc_acft_intent_group_format_text,
			.format_json = la_adsc_acft_intent_group_format_json,
			.destroy = NULL
		}
	},
	{
		.id = 0,
		.val = NULL
	}
};

/*****************
 * Tag destructors
 *****************/

static void la_adsc_tag_destroy(void *tag) {
	if(tag == NULL)
		return;
	la_adsc_tag_t *t = tag;
	if(t->data == NULL || t->type == NULL) {
		LA_XFREE(tag);
		return;
	}
	if(t->type->destroy != NULL)
		// simple types do not have any special destructors
		// and can be freed directly
		t->type->destroy(t->data);
	else
		LA_XFREE(t->data);
	LA_XFREE(tag);
}

static void la_adsc_contract_request_destroy(void *data) {
	if(data == NULL) return;
	la_adsc_req_t *r = data;
	if(r->req_tag_list != NULL) {
		la_list_free_full(r->req_tag_list, la_adsc_tag_destroy);
		r->req_tag_list = NULL;
	}
	LA_XFREE(data);
}

/****************
 * Tag formatters
 ****************/

LA_ADSC_FORMATTER_FUN(la_adsc_empty_tag_format_text) {
	LA_UNUSED(data);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "%s\n", label);
}

LA_ADSC_FORMATTER_FUN(la_adsc_empty_tag_format_json) {
	LA_UNUSED(label);
	LA_UNUSED(data);
	LA_UNUSED(ctx);
	// NOOP
}

LA_ADSC_FORMATTER_FUN(la_adsc_tag_with_contract_number_format_text) {
	LA_ISPRINTF(ctx->vstr, ctx->indent, "%s:\n", label);
	ctx->indent++;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Contract number: %u\n", *(uint8_t *)data);
	ctx->indent--;
}

LA_ADSC_FORMATTER_FUN(la_adsc_tag_with_contract_number_format_json) {
	LA_UNUSED(label);
	la_json_append_int64(ctx->vstr, "contract_num", *(uint8_t *)data);
}

LA_ADSC_FORMATTER_FUN(la_adsc_modulus_format_text) {
	LA_ISPRINTF(ctx->vstr, ctx->indent, "%s: every %u reports\n", label, *(uint8_t *)data);
}

LA_ADSC_FORMATTER_FUN(la_adsc_modulus_format_json) {
	LA_UNUSED(label);
	la_json_append_int64(ctx->vstr, "modulus", *(uint8_t *)data);
}

LA_ADSC_FORMATTER_FUN(la_adsc_reporting_interval_format_text) {
	la_adsc_report_interval_req_t const *t = data;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "%s: %d seconds\n", label,
			(int)(t->scaling_factor) * ((int)(t->rate) + 1));
}

LA_ADSC_FORMATTER_FUN(la_adsc_reporting_interval_format_json) {
	LA_UNUSED(label);
	la_adsc_report_interval_req_t const *t = data;
	la_json_append_int64(ctx->vstr, "interval_secs",
			(int)(t->scaling_factor) * ((int)(t->rate) + 1));
}

LA_ADSC_FORMATTER_FUN(la_adsc_acft_intent_group_format_text) {
	la_adsc_acft_intent_group_req_t const *t = data;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "%s: every %u reports, projection time: %u minutes\n",
			label, t->modulus, t->acft_intent_projection_time);
}

LA_ADSC_FORMATTER_FUN(la_adsc_acft_intent_group_format_json) {
	LA_UNUSED(label);
	la_adsc_acft_intent_group_req_t const *t = data;
	la_json_append_int64(ctx->vstr, "modulus", t->modulus);
	la_json_append_int64(ctx->vstr, "proj_time_mins", t->acft_intent_projection_time);
}

LA_ADSC_FORMATTER_FUN(la_adsc_lat_dev_change_format_text) {
	la_adsc_lat_dev_chg_event_t const *e = data;
	LA_ISPRINTF(ctx->vstr, ctx->indent,
			"%s: %.3f nm\n",
			label,
			e->lat_dev_threshold
			);
}

LA_ADSC_FORMATTER_FUN(la_adsc_lat_dev_change_format_json) {
	LA_UNUSED(label);
	la_adsc_lat_dev_chg_event_t const *e = data;
	la_json_append_double(ctx->vstr, "lat_dev_treshold_nm", e->lat_dev_threshold);
}

LA_ADSC_FORMATTER_FUN(la_adsc_vspd_change_format_text) {
	la_adsc_vspd_chg_event_t const *e = data;
	LA_ISPRINTF(ctx->vstr, ctx->indent,
			"%s: %c%d ft/min\n",
			label,
			e->vspd_threshold >= 0 ? '>' : '<',
			abs(e->vspd_threshold)
			);
}

LA_ADSC_FORMATTER_FUN(la_adsc_vspd_change_format_json) {
	LA_UNUSED(label);
	la_adsc_vspd_chg_event_t const *e = data;
	la_json_append_int64(ctx->vstr, "vspd_ftmin_threshold", abs(e->vspd_threshold));
	la_json_append_bool(ctx->vstr, "higher_than", e->vspd_threshold >= 0 ? true : false);
}

LA_ADSC_FORMATTER_FUN(la_adsc_alt_range_format_text) {
	la_adsc_alt_range_event_t const *e = data;
	LA_ISPRINTF(ctx->vstr, ctx->indent,
			"%s: %d-%d ft\n",
			label,
			e->floor_alt,
			e->ceiling_alt
			);
}

LA_ADSC_FORMATTER_FUN(la_adsc_alt_range_format_json) {
	LA_UNUSED(label);
	la_adsc_alt_range_event_t const *e = data;
	la_json_append_int64(ctx->vstr, "floor_alt", e->floor_alt);
	la_json_append_int64(ctx->vstr, "ceiling_alt", e->ceiling_alt);
}

LA_ADSC_FORMATTER_FUN(la_adsc_contract_request_format_text) {
	la_adsc_req_t const *r = data;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "%s:\n", label);
	ctx->indent++;
	LA_ISPRINTF(ctx->vstr, ctx->indent, "Contract number: %u\n", r->contract_num);

	size_t len = la_list_length(r->req_tag_list);
	if(len == 0) {
		return;
	}
	for(la_list *ptr = r->req_tag_list; ptr != NULL; ptr = la_list_next(ptr)) {
		la_adsc_tag_t *t = ptr->data;
		if(!t->type) {
			LA_ISPRINTF(ctx->vstr, ctx->indent, "-- Unparseable tag %u\n", t->tag);
			break;
		}
		la_assert(t->type->format_text != NULL);
		t->type->format_text(ctx, t->type->label, t->data);
	}
	ctx->indent--;
}

LA_ADSC_FORMATTER_FUN(la_adsc_contract_request_format_json) {
	LA_UNUSED(label);
	la_adsc_req_t const *r = data;
	la_json_append_int64(ctx->vstr, "contract_num", r->contract_num);

	size_t len = la_list_length(r->req_tag_list);
	if(len == 0) {
		return;
	}
	la_json_array_start(ctx->vstr, "groups");
	for(la_list *ptr = r->req_tag_list; ptr != NULL; ptr = la_list_next(ptr)) {
		la_adsc_tag_t *t = ptr->data;
		if(t->type == NULL || t->type->format_json == NULL || t->type->json_key == NULL) {
			break;
		}
		la_json_object_start(ctx->vstr, NULL);
		la_json_object_start(ctx->vstr, t->type->json_key);
		t->type->format_json(ctx, NULL, t->data);
		la_json_object_end(ctx->vstr);
		la_json_object_end(ctx->vstr);
	}
	la_json_array_end(ctx->vstr);
}

/**************
 * Tag parsers
 **************/

LA_ADSC_PARSER_FUN(la_adsc_uint8_t_parse) {
	uint32_t tag_len = 1;
	la_adsc_tag_t *t = dest;
	LA_ADSC_CHECK_LEN(t->tag, len, tag_len);
	LA_NEW(uint8_t, ptr);
	*ptr = buf[0];
	la_debug_print(D_INFO, "val=%u\n", *ptr);
	t->data = ptr;
	return tag_len;
}

LA_ADSC_PARSER_FUN(la_adsc_reporting_interval_parse) {
	uint32_t tag_len = 1;
	la_adsc_tag_t *t = dest;
	LA_ADSC_CHECK_LEN(t->tag, len, tag_len);
	LA_NEW(la_adsc_report_interval_req_t, ri);
	t->data = ri;
	uint8_t sf = (buf[0] & 0xc0) >> 6;
	// convert scaling factor to multiplier value
	if(sf == 2)
		sf = 8;
	else if(sf == 3)
		sf = 64;
	ri->scaling_factor = sf;
	ri->rate = buf[0] & 0x3f;
	la_debug_print(D_INFO, "SF=%u rate=%u\n", ri->scaling_factor, ri->rate);
	return tag_len;
}

LA_ADSC_PARSER_FUN(la_adsc_lat_dev_change_parse) {
	uint32_t tag_len = 1;
	la_adsc_tag_t *t = dest;
	LA_ADSC_CHECK_LEN(t->tag, len, tag_len);
	LA_NEW(la_adsc_lat_dev_chg_event_t, e);
	t->data = e;

	e->lat_dev_threshold = (double)buf[0] / 8.0;
	return tag_len;
}

LA_ADSC_PARSER_FUN(la_adsc_vspd_change_parse) {
	uint32_t tag_len = 1;
	la_adsc_tag_t *t = dest;
	LA_ADSC_CHECK_LEN(t->tag, len, tag_len);
	LA_NEW(la_adsc_vspd_chg_event_t, e);
	t->data = e;

	struct { signed int vt:8; } s;
	e->vspd_threshold = s.vt = (int)buf[0];
	e->vspd_threshold *= 64;
	la_debug_print(D_INFO, "result: %d\n", e->vspd_threshold);
	return tag_len;
}

LA_ADSC_PARSER_FUN(la_adsc_alt_range_parse) {
	uint32_t tag_len = 4;
	la_adsc_tag_t *t = dest;
	LA_ADSC_CHECK_LEN(t->tag, len, tag_len);
	LA_NEW(la_adsc_alt_range_event_t, e);
	t->data = e;

	uint32_t tmp = 0;
	tmp = (buf[0] << 8) | buf[1];
	e->ceiling_alt = la_adsc_altitude_parse(tmp);
	tmp = (buf[2] << 8) | buf[3];
	e->floor_alt = la_adsc_altitude_parse(tmp);
	return tag_len;
}

LA_ADSC_PARSER_FUN(la_adsc_acft_intent_group_parse) {
	uint32_t tag_len = 2;
	la_adsc_tag_t *t = dest;
	LA_ADSC_CHECK_LEN(t->tag, len, tag_len);
	LA_NEW(la_adsc_acft_intent_group_req_t, aig);
	t->data = aig;
	aig->modulus = buf[0];
	aig->acft_intent_projection_time = buf[1];
	la_debug_print(D_INFO, "modulus=%u projection_time=%u\n", aig->modulus, aig->acft_intent_projection_time);
	return tag_len;
}


LA_ADSC_PARSER_FUN(la_adsc_contract_request_parse) {
	uint32_t tag_len = 1;
	la_adsc_tag_t *t = dest;
	LA_ADSC_CHECK_LEN(t->tag, len, tag_len);
	int consumed_bytes = 0;
	LA_NEW(la_adsc_req_t, r);
	t->data = r;

	r->contract_num = buf[0];
	buf++; len--;

	while(len > 0) {
		la_debug_print(D_INFO, "Remaining length: %u\n", len);
		// First lookup the tag value - if it's unknown, then it's probably a next request
		// in a multi-request ADS message. We don't want la_adsc_tag_parse() to parse it,
		// because we would get a nasty "-- Unparseable tag" error message in the output.
		if(la_dict_search(la_adsc_request_tag_descriptor_table, (int)buf[0]) == NULL) {
			la_debug_print(D_INFO, "Tag %d unknown - assuming end-of-request\n", (int)buf[0]);
			break;
		}
		LA_NEW(la_adsc_tag_t, req_tag);
		r->req_tag_list = la_list_append(r->req_tag_list, req_tag);
		if((consumed_bytes = la_adsc_tag_parse(req_tag, la_adsc_request_tag_descriptor_table, buf, len)) < 0) {
			return -1;
		}
		buf += consumed_bytes; len -= consumed_bytes;
		tag_len += consumed_bytes;
	}
	return tag_len;
}

static int la_adsc_tag_parse(la_adsc_tag_t *t, la_dict const
		*tag_descriptor_table, uint8_t const *buf, uint32_t len) {
	uint32_t tag_len = 1;
	if(len < tag_len) {
		la_debug_print(D_INFO, "Buffer len is 0\n");
		return -1;
	}

	t->tag = buf[0];
	buf++; len--;
	la_adsc_type_descriptor_t *type = la_dict_search(tag_descriptor_table, (int)t->tag);
	if(type == NULL) {
		la_debug_print(D_ERROR, "Unknown tag %u\n", t->tag);
		return -1;
	}
	la_debug_print(D_INFO, "Found tag %u (%s)\n", t->tag, type->label);
	int consumed_bytes = 0;
	if(type->parse == NULL) {       // tag is empty, no parsing required - return with success
		goto end;
	}
	if((consumed_bytes = (*(type->parse))(t, buf, len)) < 0) {
		return -1;
	}
end:
	tag_len += consumed_bytes;
	t->type = type;
	return tag_len;
}

la_proto_node *la_adsc_parse(uint8_t const *buf, int len, la_msg_dir msg_dir, la_arinc_imi imi) {
	if(buf == NULL)
		return NULL;

	la_proto_node *node = la_proto_node_new();
	LA_NEW(la_adsc_msg_t, msg);
	node->data = msg;
	node->td = &la_DEF_adsc_message;
	la_adsc_tag_t *tag = NULL;
	int consumed_bytes;

	// Uplink and downlink tag values are the same, but their syntax is different.
	// Figure out the dictionary to use based on the message direction.
	static la_dict const *tag_table = NULL;
	if(msg_dir == LA_MSG_DIR_GND2AIR)
		tag_table = la_adsc_uplink_tag_descriptor_table;
	else if(msg_dir == LA_MSG_DIR_AIR2GND)
		tag_table = la_adsc_downlink_tag_descriptor_table;
	la_assert(tag_table != NULL);

	msg->err = false;
	switch(imi) {
		case ARINC_MSG_ADS:
			while(len > 0) {
				la_debug_print(D_INFO, "Remaining length: %u\n", len);
				tag = LA_XCALLOC(1, sizeof(la_adsc_tag_t));
				msg->tag_list = la_list_append(msg->tag_list, tag);
				if((consumed_bytes = la_adsc_tag_parse(tag, tag_table, buf, len)) < 0) {
					msg->err = true;
					break;
				}
				buf += consumed_bytes; len -= consumed_bytes;
			}
			break;
		case ARINC_MSG_DIS:
			// DIS payload consists of an error code only, without any tag.
			// Let's insert a fake tag value of 255.
			if(len < 1) {
				la_debug_print(D_ERROR, "DIS message too short\n");
				msg->err = true;
				break;
			}
			tag = LA_XCALLOC(1, sizeof(la_adsc_tag_t));
			msg->tag_list = la_list_append(msg->tag_list, tag);
			len = 2;
			uint8_t *tmpbuf = LA_XCALLOC(len, sizeof(uint8_t));
			tmpbuf[0] = 255;
			tmpbuf[1] = buf[0];
			if(la_adsc_tag_parse(tag, tag_table, tmpbuf, len) < 0) {
				msg->err = true;
			}
			LA_XFREE(tmpbuf);
			break;
		default:
			break;
	}
	return node;
}

static void la_adsc_tag_output_text(void const *p, void *ctx) {
	la_assert(p);
	la_assert(ctx);

	la_adsc_tag_t const *t = p;
	la_adsc_formatter_ctx_t *c = ctx;
	if(!t->type) {
		LA_ISPRINTF(c->vstr, c->indent, "-- Unparseable tag %u\n", t->tag);
		return;
	}
	if(t->type->format_text != NULL) {
		t->type->format_text(c, t->type->label, t->data);
	}
}

static void la_adsc_tag_output_json(void const *p, void *ctx) {
	la_assert(p);
	la_assert(ctx);

	la_adsc_tag_t const *t = p;
	la_adsc_formatter_ctx_t *c = ctx;
	if(!t->type) {
		return;
	}
	if(t->type->format_json != NULL && t->type->json_key != NULL) {
		// Every tags gets encapsulated in a separate anonymous object,
		// so that correct JSON output is produced even if the message
		// contains duplicate tags.
		la_json_object_start(c->vstr, NULL);
		la_json_object_start(c->vstr, t->type->json_key);
		t->type->format_json(c, t->type->json_key, t->data);
		la_json_object_end(c->vstr);
		la_json_object_end(c->vstr);
	}
}

void la_adsc_format_text(la_vstring *vstr, void const *data, int indent) {
	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	la_adsc_msg_t const *msg = data;
	la_adsc_formatter_ctx_t ctx = {
		.vstr = vstr,
		.indent = indent
	};
	if(msg->tag_list == NULL) {
		LA_ISPRINTF(ctx.vstr, ctx.indent, "-- Empty ADS-C message\n");
		return;
	}
	la_list_foreach(msg->tag_list, la_adsc_tag_output_text, &ctx);
	if(msg->err == true) {
		LA_ISPRINTF(ctx.vstr, ctx.indent, "-- Malformed ADS-C message\n");
	}
}

void la_adsc_format_json(la_vstring *vstr, void const *data) {
	la_assert(vstr);
	la_assert(data);

	la_adsc_msg_t const *msg = data;
	if(msg->tag_list == NULL) {
		return;
	}
	la_adsc_formatter_ctx_t ctx = {
		.vstr = vstr,
		.indent = 0     // unused in JSON output
	};
	la_json_array_start(vstr, "tags");
	la_list_foreach(msg->tag_list, la_adsc_tag_output_json, &ctx);
	la_json_array_end(vstr);
	la_json_append_bool(vstr, "err", msg->err);
}

void la_adsc_destroy(void *data) {
	if(data == NULL) {
		return;
	}
	la_adsc_msg_t *msg = data;
	la_list_free_full(msg->tag_list, la_adsc_tag_destroy);
	msg->tag_list = NULL;
	msg->err = false;
	LA_XFREE(data);
}

la_type_descriptor const la_DEF_adsc_message = {
	.format_text = la_adsc_format_text,
	.format_json = la_adsc_format_json,
	.json_key = "adsc",
	.destroy = la_adsc_destroy
};

la_proto_node *la_proto_tree_find_adsc(la_proto_node *root) {
	return la_proto_tree_find_protocol(root, &la_DEF_adsc_message);
}
