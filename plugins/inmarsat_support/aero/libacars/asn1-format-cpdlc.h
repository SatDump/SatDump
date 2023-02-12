/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifndef LA_ASN1_FORMAT_CPDLC_H
#define LA_ASN1_FORMAT_CPDLC_H 1
#include <aero/libacars/asn1/asn_application.h>      // asn_TYPE_descriptor_t
#include <aero/libacars/vstring.h>                   // la_vstring
#include <aero/libacars/dict.h>                      // la_dict
#include <aero/libacars/asn1-util.h>                 // LA_ASN1_FORMATTER_FUNC

// asn1-format-cpdlc-text.c
LA_ASN1_FORMATTER_FUNC(la_asn1_output_cpdlc_as_text);
extern la_dict const FANSATCUplinkMsgElementId_labels[];
extern la_dict const FANSATCDownlinkMsgElementId_labels[];

// asn1-format-cpdlc-json.c
LA_ASN1_FORMATTER_FUNC(la_asn1_output_cpdlc_as_json);

#endif // !LA_ASN1_FORMAT_CPDLC_H
