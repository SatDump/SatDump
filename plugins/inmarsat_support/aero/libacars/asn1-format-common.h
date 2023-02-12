/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifndef LA_ASN1_FORMAT_COMMON_H
#define LA_ASN1_FORMAT_COMMON_H 1
#include <aero/libacars/asn1/asn_application.h>  // asn_TYPE_descriptor_t
#include <aero/libacars/asn1-util.h>             // LA_ASN1_FORMATTER_FUNC, la_asn1_formatter_params
#include <aero/libacars/dict.h>                  // la_dict
#include <aero/libacars/vstring.h>               // la_vstring

char const *la_asn1_value2enum(asn_TYPE_descriptor_t *td, long value);
void la_format_INTEGER_with_unit_as_text(la_asn1_formatter_params p,
		char const *unit, double multiplier, int decimal_places);
void la_format_CHOICE_as_text(la_asn1_formatter_params p, la_dict const *choice_labels,
		la_asn1_formatter_func cb);
void la_format_INTEGER_as_ENUM_as_text(la_asn1_formatter_params p, la_dict const *value_labels);
void la_format_SEQUENCE_as_text(la_asn1_formatter_params p, la_asn1_formatter_func cb);
void la_format_SEQUENCE_OF_as_text(la_asn1_formatter_params p, la_asn1_formatter_func cb);
void la_format_BIT_STRING_as_text(la_asn1_formatter_params p, la_dict const *bit_labels);

LA_ASN1_FORMATTER_FUNC(la_asn1_format_any_as_text);
LA_ASN1_FORMATTER_FUNC(la_asn1_format_ENUM_as_text);
LA_ASN1_FORMATTER_FUNC(la_asn1_format_label_only_as_text);

void la_format_INTEGER_with_unit_as_json(la_asn1_formatter_params p,
		char const *unit, double multiplier);
void la_format_CHOICE_as_json(la_asn1_formatter_params p, la_dict const *choice_labels,
		la_asn1_formatter_func cb);
void la_format_INTEGER_as_ENUM_as_json(la_asn1_formatter_params p, la_dict const *value_labels);
void la_format_SEQUENCE_as_json(la_asn1_formatter_params p, la_asn1_formatter_func cb);
void la_format_SEQUENCE_OF_as_json(la_asn1_formatter_params p, la_asn1_formatter_func cb);
void la_format_BIT_STRING_as_json(la_asn1_formatter_params p, la_dict const *bit_labels);

LA_ASN1_FORMATTER_FUNC(la_asn1_format_any_as_string_as_json);
LA_ASN1_FORMATTER_FUNC(la_asn1_format_ENUM_as_json);
LA_ASN1_FORMATTER_FUNC(la_asn1_format_label_only_as_json);
LA_ASN1_FORMATTER_FUNC(la_asn1_format_long_as_json);
LA_ASN1_FORMATTER_FUNC(la_asn1_format_bool_as_json);
LA_ASN1_FORMATTER_FUNC(la_asn1_format_OCTET_STRING_as_json);

#endif // !LA_ASN1_FORMAT_COMMON_H
