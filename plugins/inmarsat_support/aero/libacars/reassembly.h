/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */
#ifndef LA_REASSEMBLY_H
#define LA_REASSEMBLY_H 1

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#ifndef _MSC_VER
#include <sys/time.h>
#else
#include <winsock.h>
#endif
#include <aero/libacars/hash.h>

typedef struct la_reasm_ctx_s la_reasm_ctx;
typedef struct la_reasm_table_s la_reasm_table;

typedef void *(la_reasm_get_key_func)(void const *msg);
typedef la_hash_func la_reasm_hash_func;
typedef la_hash_compare_func la_reasm_compare_func;
typedef la_hash_key_destroy_func la_reasm_key_destroy_func;
typedef struct {
	la_reasm_get_key_func *get_key;
	la_reasm_get_key_func *get_tmp_key;
	la_reasm_hash_func *hash_key;
	la_reasm_compare_func *compare_keys;
	la_reasm_key_destroy_func *destroy_key;
} la_reasm_table_funcs;

#define SEQ_FIRST_NONE -1
#define SEQ_WRAP_NONE -1

typedef struct {
	void const *msg_info;           /* pointer to message metadata (eg. header),
	                                   used as hash key */

	uint8_t *msg_data;              /* packet data buffer */

	int msg_data_len;               /* packet data buffer length */

	int total_pdu_len;              /* Total length of the reassembled message.
                                       If > 0, then reassembly is completed when
                                       total length of collected fragments reaches
                                       this value. If <= 0, then caller must signal
                                       the final fragment using is_final_fragment flag. */

	struct timeval rx_time;         /* fragment receive timestamp */

	struct timeval reasm_timeout;   /* reassembly timeout to be applied to this message */

	int seq_num;                    /* sequence number of this fragment (non-negative) */

	int seq_num_first;              /* this sequence number indicates the first fragment
	                                   of the message (SEQ_FIRST_NONE if there is no
	                                   indication of the first fragment) */

	int seq_num_wrap;               /* the value at which the sequence number wraps
	                                   to 0 (SEQ_WRAP_NONE if it doesn't wrap) */

	bool is_final_fragment;         /* is this the final fragment of this message? */

// reserved for future use
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
} la_reasm_fragment_info;

typedef enum {
	LA_REASM_UNKNOWN,
	LA_REASM_COMPLETE,
	LA_REASM_IN_PROGRESS,
	LA_REASM_SKIPPED,
	LA_REASM_DUPLICATE,
	LA_REASM_FRAG_OUT_OF_SEQUENCE,
	LA_REASM_ARGS_INVALID
} la_reasm_status;
#define LA_REASM_STATUS_MAX LA_REASM_ARGS_INVALID

// reassembly.c
la_reasm_ctx *la_reasm_ctx_new();
void la_reasm_ctx_destroy(void *ctx);
la_reasm_table *la_reasm_table_new(la_reasm_ctx *rctx, void const *table_id,
		la_reasm_table_funcs funcs, int cleanup_interval);
la_reasm_table *la_reasm_table_lookup(la_reasm_ctx *rctx, void const *table_id);
la_reasm_status la_reasm_fragment_add(la_reasm_table *rtable, la_reasm_fragment_info const *finfo);
int la_reasm_payload_get(la_reasm_table *rtable, void const *msg_info, uint8_t **result);
char const *la_reasm_status_name_get(la_reasm_status status);

#ifdef __cplusplus
}
#endif

#endif // !LA_REASSEMBLY_H
