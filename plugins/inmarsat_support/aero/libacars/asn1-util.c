/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#include <stdint.h>
#include <search.h>                         // lfind()
#include <aero/libacars/asn1/asn_application.h>  // asn_TYPE_descriptor_t
#include <aero/libacars/asn1-util.h>             // la_asn1_formatter
#include <aero/libacars/macros.h>                // LA_ISPRINTF, la_debug_print
#include <aero/libacars/vstring.h>               // la_vstring
#include "config.h"                         // LFIND_NMEMB_SIZE_SIZE_T, LFIND_NMEMB_SIZE_UINT

static int la_compare_fmtr(void const *k, void const *m) {
	la_asn1_formatter const *memb = m;
	return(k == memb->type ? 0 : 1);
}

int la_asn1_decode_as(asn_TYPE_descriptor_t *td, void **struct_ptr, uint8_t const *buf, int size) {
	asn_dec_rval_t rval;
	rval = uper_decode_complete(0, td, struct_ptr, buf, size);
	if(rval.code != RC_OK) {
		la_debug_print(D_ERROR, "uper_decode_complete failed: %d\n", rval.code);
		return -1;
	}
	if(rval.consumed < (size_t)size) {
		la_debug_print(D_ERROR, "uper_decode_complete left %zd unparsed octets\n", (size_t)size - rval.consumed);
		return (int)((size_t)size - rval.consumed);
	}
#ifdef DEBUG
	if(Debug >= D_VERBOSE) {
		asn_fprint(stderr, td, *struct_ptr, 1);
	}
#endif
	return 0;
}

void la_asn1_output(la_asn1_formatter_params p, la_asn1_formatter const *asn1_formatter_table,
		size_t asn1_formatter_table_len, bool dump_unknown_types) {
	if(p.td == NULL || p.sptr == NULL) return;
#if defined LFIND_NMEMB_SIZE_SIZE_T
	size_t table_len = asn1_formatter_table_len;
#elif defined LFIND_NMEMB_SIZE_UINT
	unsigned int table_len = (unsigned int)asn1_formatter_table_len;
#endif
	la_asn1_formatter *formatter = lfind(p.td, asn1_formatter_table, &table_len,
			sizeof(la_asn1_formatter), &la_compare_fmtr);
	if(formatter != NULL) {
		// NULL formatting routine is allowed - it means the type should be silently omitted
		if(formatter->format != NULL) {
			p.label = formatter->label;
			(*formatter->format)(p);
		}
	} else if(dump_unknown_types) {
		LA_ISPRINTF(p.vstr, p.indent, "-- Formatter for type %s not found, ASN.1 dump follows:\n", p.td->name);
		LA_ISPRINTF(p.vstr, p.indent, "%s", "");    // asn_sprintf does not indent the first line
		asn_sprintf(p.vstr, p.td, p.sptr, p.indent+1);
		LA_EOL(p.vstr);
		LA_ISPRINTF(p.vstr, p.indent, "%s", "-- ASN.1 dump end\n");
	}
}
