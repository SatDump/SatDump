/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

/* Default libacars configuration settings */

{
// Output raw dump of ASN.1 structure for all ASN.1-encoded messages?

	LA_CONFIG_SETTING_BOOLEAN("dump_asn1", false),

// Try to decode ACARS applications in fragmented messages, when
// reassembly is in progress?
// If reassembly is disabled, this setting has no effect (ie. applications
// are always decoded).
//
	LA_CONFIG_SETTING_BOOLEAN("decode_fragments", false),

// Radio link type (bearer) used to transmit ACARS messages processed by
// libacars. Must be set correctly when message reassembly is used (it
// determines timer values for the reassembly process). Default is 1 (VHF).
// See acars.h for full list of supported values.

	LA_CONFIG_SETTING_INTEGER("acars_bearer", 1),

// Pretty-print XML in ACARS and MIAM Core payloads?

	LA_CONFIG_SETTING_BOOLEAN("prettify_xml", false)
};
