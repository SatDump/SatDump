/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifdef WIN32
#else
#include <sys/time.h>                   // struct timeval
#endif
#include <string.h>                     // strdup
#include <aero/libacars/macros.h>            // la_assert
#include <aero/libacars/hash.h>              // la_hash
#include <aero/libacars/list.h>              // la_list
#include <aero/libacars/util.h>              // LA_XCALLOC, LA_XFREE, la_octet_string
#include <aero/libacars/reassembly.h>

typedef struct la_reasm_table_s {
	void const *key;                    /* a pointer identifying the protocol
	                                       owning this reasm_table (la_type_descriptor
	                                       can be used for this purpose). Due to small
	                                       number of protocols, hash would be an overkill
	                                       here. */
	la_hash *fragment_table;            /* keyed with packet identifiers, values are
	                                       la_reasm_table_entries */
	la_reasm_table_funcs funcs;         /* protocol-specific callbacks */
	int cleanup_interval;               /* expire old entries every cleanup_interval
	                                       number of processed fragments */
	int frag_cnt;                       /* counts added fragments (up to cleanup_interval) */
} la_reasm_table;

struct la_reasm_ctx_s {
	la_list *rtables;                   /* list of reasm_tables, one per protocol */
};

// the header of the fragment list
typedef struct {
	int prev_seq_num;                   /* sequence number of previous fragment */

	int frags_collected_total_len;      /* sum of msg_data_len for all fragments received */

	int total_pdu_len;                  /* total length of the reassembled message
	                                       (copied from la_reasm_fragment_info of the 1st fragment) */

	struct timeval first_frag_rx_time;  /* time of arrival of the first fragment */

	struct timeval reasm_timeout;       /* reassembly timeout to be applied to this message */

	la_list *fragment_list;             /* payloads of all fragments gathered so far */
} la_reasm_table_entry;

la_reasm_ctx *la_reasm_ctx_new() {
	LA_NEW(la_reasm_ctx, rctx);
	return rctx;
}

static void la_reasm_table_entry_destroy(void *rt_ptr) {
	if(rt_ptr == NULL) {
		return;
	}
	la_reasm_table_entry *rt_entry = rt_ptr;
	la_list_free_full(rt_entry->fragment_list, la_octet_string_destroy);
	LA_XFREE(rt_entry);
}

static void la_reasm_table_destroy(void *table) {
	if(table == NULL) {
		return;
	}
	la_reasm_table *rtable = table;
	la_hash_destroy(rtable->fragment_table);
	LA_XFREE(rtable);
}

void la_reasm_ctx_destroy(void *ctx) {
	if(ctx == NULL) {
		return;
	}
	la_reasm_ctx *rctx = ctx;
	la_list_free_full(rctx->rtables, la_reasm_table_destroy);
	LA_XFREE(rctx);
}

la_reasm_table *la_reasm_table_lookup(la_reasm_ctx *rctx, void const *table_id) {
	la_assert(rctx != NULL);
	la_assert(table_id != NULL);

	for(la_list *l = rctx->rtables; l != NULL; l = la_list_next(l)) {
		la_reasm_table *rt = l->data;
		if(rt->key == table_id) {
			return rt;
		}
	}
	return NULL;
}

#define LA_REASM_DEFAULT_CLEANUP_INTERVAL 100

la_reasm_table *la_reasm_table_new(la_reasm_ctx *rctx, void const *table_id,
		la_reasm_table_funcs funcs, int cleanup_interval) {
	la_assert(rctx != NULL);
	la_assert(table_id != NULL);
	la_assert(funcs.get_key);
	la_assert(funcs.get_tmp_key);
	la_assert(funcs.hash_key);
	la_assert(funcs.compare_keys);
	la_assert(funcs.destroy_key);

	la_reasm_table *rtable = la_reasm_table_lookup(rctx, table_id);
	if(rtable != NULL) {
		goto end;
	}
	rtable = LA_XCALLOC(1, sizeof(la_reasm_table));
	rtable->key = table_id;
	rtable->fragment_table = la_hash_new(funcs.hash_key, funcs.compare_keys,
			funcs.destroy_key, la_reasm_table_entry_destroy);
	rtable->funcs = funcs;

	// Replace insane values with reasonable default
	rtable->cleanup_interval = cleanup_interval > 0 ?
		cleanup_interval : LA_REASM_DEFAULT_CLEANUP_INTERVAL;
	rctx->rtables = la_list_append(rctx->rtables, rtable);
end:
	return rtable;
}

// Checks if time difference between rx_first and rx_last is greater than timeout.
static bool la_reasm_timed_out(struct timeval rx_last, struct timeval rx_first,
		struct timeval timeout) {
	if(timeout.tv_sec == 0 && timeout.tv_usec == 0) {
		return false;
	}
	struct timeval to = {
		.tv_sec = rx_first.tv_sec + timeout.tv_sec,
		.tv_usec = rx_first.tv_usec + timeout.tv_usec
	};
	if(to.tv_usec > 1e9) {
		to.tv_sec++;
		to.tv_usec -= 1e9;
	}
	la_debug_print(D_INFO, "rx_first: %lu.%lu to: %lu.%lu rx_last: %lu.%lu\n",
			rx_first.tv_sec, rx_first.tv_usec, to.tv_sec, to.tv_usec, rx_last.tv_sec, rx_last.tv_usec);
	return (rx_last.tv_sec > to.tv_sec ||
			(rx_last.tv_sec == to.tv_sec && rx_last.tv_usec > to.tv_usec));
}

// Callback for la_hash_foreach_remove used during reassembly table cleanups.
static bool is_rt_entry_expired(void const *keyptr, void const *valptr, void *ctx) {
	LA_UNUSED(keyptr);
	la_assert(valptr != NULL);
	la_assert(ctx != NULL);

	la_reasm_table_entry const *rt_entry = valptr;
	struct timeval *now = ctx;
	return la_reasm_timed_out(*now, rt_entry->first_frag_rx_time, rt_entry->reasm_timeout);
}

// Removes expired entries from the given reassembly table.
static void la_reasm_table_cleanup(la_reasm_table *rtable, struct timeval now) {
	la_assert(rtable != NULL);
	la_assert(rtable->fragment_table != NULL);
	int deleted_count = la_hash_foreach_remove(rtable->fragment_table,
			is_rt_entry_expired, &now);
	// Avoid compiler warning when DEBUG is off
	LA_UNUSED(deleted_count);
	la_debug_print(D_INFO, "Expired %d entries\n", deleted_count);
}

#define SEQ_UNINITIALIZED -2

// Checks if the given sequence number follows the previous one seen.
static bool is_seq_num_in_sequence(int prev_seq_num, int cur_seq_num) {
	return (prev_seq_num == SEQ_UNINITIALIZED || prev_seq_num + 1 == cur_seq_num);
}

// Core reassembly logic.
// Validates the given message fragment and appends it to the reassembly table
// fragment list.
la_reasm_status la_reasm_fragment_add(la_reasm_table *rtable, la_reasm_fragment_info const *finfo) {
	la_assert(rtable != NULL);
	la_assert(finfo != NULL);
	if(finfo->msg_info == NULL) {
		return LA_REASM_ARGS_INVALID;
	}

	// Don't allow zero timeout. This would prevent stale rt_entries from being expired,
	// causing a massive memory leak.

	if(finfo->reasm_timeout.tv_sec == 0 && finfo->reasm_timeout.tv_usec == 0) {
		return LA_REASM_ARGS_INVALID;
	}

	la_reasm_status ret = LA_REASM_UNKNOWN;
	void *lookup_key = rtable->funcs.get_tmp_key(finfo->msg_info);
	la_assert(lookup_key != NULL);
	la_reasm_table_entry *rt_entry = NULL;
restart:
	rt_entry = la_hash_lookup(rtable->fragment_table, lookup_key);
	if(rt_entry == NULL) {

		// Don't add if we know that this is not the first fragment of the message.

		if(finfo->seq_num_first != SEQ_FIRST_NONE && finfo->seq_num_first != finfo->seq_num) {
			la_debug_print(D_INFO, "No rt_entry found and seq_num %d != seq_num_first %d,"
					" not creating rt_entry\n", finfo->seq_num, finfo->seq_num_first);
			ret = LA_REASM_FRAG_OUT_OF_SEQUENCE;
			goto end;
		}
		if(finfo->is_final_fragment) {

			// This is the first received fragment of this message and it's the final
			// fragment.  Either this message is not fragmented or all fragments except the
			// last one have been lost.  In either case there is no point in adding it to
			// the fragment table.

			la_debug_print(D_INFO, "No rt_entry found and is_final_fragment=true, not creating rt_entry\n");
			ret = LA_REASM_SKIPPED;
			goto end;
		}
		rt_entry = LA_XCALLOC(1, sizeof(la_reasm_table_entry));
		rt_entry->prev_seq_num = SEQ_UNINITIALIZED;
		rt_entry->first_frag_rx_time = finfo->rx_time;
		rt_entry->reasm_timeout = finfo->reasm_timeout;
		rt_entry->total_pdu_len = LA_MAX(finfo->total_pdu_len, 0);
		rt_entry->frags_collected_total_len = 0;
		la_debug_print(D_INFO, "Adding new rt_table entry (rx_time: %lu.%lu timeout: %lu.%lu)\n",
				rt_entry->first_frag_rx_time.tv_sec, rt_entry->first_frag_rx_time.tv_usec,
				rt_entry->reasm_timeout.tv_sec, rt_entry->reasm_timeout.tv_usec);
		void *msg_key = rtable->funcs.get_key(finfo->msg_info);
		la_assert(msg_key != NULL);
		la_hash_insert(rtable->fragment_table, msg_key, rt_entry);
	} else {
		la_debug_print(D_INFO, "rt_entry found, prev_seq_num: %d\n", rt_entry->prev_seq_num);
	}

	// Check if the sequence number has wrapped (if we're supposed to handle wraparounds)

	if(finfo->seq_num_wrap != SEQ_WRAP_NONE && finfo->seq_num == 0 &&
			finfo->seq_num_wrap == rt_entry->prev_seq_num + 1) {
		la_debug_print(D_INFO, "seq_num wrap at %d: %d -> %d\n", finfo->seq_num_wrap,
				rt_entry->prev_seq_num, finfo->seq_num);

		// Current seq_num is 0, so set prev_seq_num to -1 to cause the seq_num check to succeed

		rt_entry->prev_seq_num = -1;
	}

	// Check reassembly timeout

	if(la_reasm_timed_out(finfo->rx_time, rt_entry->first_frag_rx_time, rt_entry->reasm_timeout) == true) {

		// If reassembly timeout has expired, we treat this fragment as a part of
		// a new message. Remove the old rt_entry and create new one.

		la_debug_print(D_INFO, "reasm timeout expired; creating new rt_entry\n");
		la_hash_remove(rtable->fragment_table, lookup_key);
		goto restart;
	}

	// Skip duplicates / retransmissions.
	// If sequence numbers don't wrap, then treat fragments we've seen before as
	// duplicates too.

	if(rt_entry->prev_seq_num == finfo->seq_num ||
			(finfo->seq_num_wrap == SEQ_WRAP_NONE && finfo->seq_num < rt_entry->prev_seq_num)) {
		la_debug_print(D_INFO, "skipping duplicate fragment (seq_num: %d)\n", finfo->seq_num);
		ret = LA_REASM_DUPLICATE;
		goto end;
	}

	// Check If the sequence number has incremented.

	if(is_seq_num_in_sequence(rt_entry->prev_seq_num, finfo->seq_num) == false) {

		// Probably one or more fragments have been lost. Reassembly is not possible.

		la_debug_print(D_INFO, "seq_num %d out of sequence (prev: %d)\n",
				finfo->seq_num, rt_entry->prev_seq_num);
		la_hash_remove(rtable->fragment_table, lookup_key);
		ret = LA_REASM_FRAG_OUT_OF_SEQUENCE;
		goto end;
	}

	// All checks succeeded. Add the fragment to the list.

	la_debug_print(D_INFO, "Good seq_num %d (prev: %d), adding fragment to the list\n",
			finfo->seq_num, rt_entry->prev_seq_num);
	// Don't append fragments with empty payload (but increment seq_num anyway,
	// because empty fragment is not an error)
	if(finfo->msg_data != NULL && finfo->msg_data_len > 0) {
		uint8_t *msg_data = LA_XCALLOC(finfo->msg_data_len, sizeof(uint8_t));
		memcpy(msg_data, finfo->msg_data, finfo->msg_data_len);
		la_octet_string *ostring = la_octet_string_new(msg_data, finfo->msg_data_len);
		rt_entry->fragment_list = la_list_append(rt_entry->fragment_list, ostring);
	}
	rt_entry->frags_collected_total_len += finfo->msg_data_len;
	rt_entry->prev_seq_num = finfo->seq_num;

	// If we've come to this point successfully, then reassembly is complete if:
	//
	// - total_pdu_len for this rt_entry is set and we've already collected
	//   required amount of data, or
	//
	// - total_pdu_len for this rt_entry is not known and the caller indicates
	//   that this is the final fragment of this message.
	//
	// Otherwise we expect more fragments to come.

	if(rt_entry->total_pdu_len > 0) {
		ret = rt_entry->frags_collected_total_len >= rt_entry->total_pdu_len ?
			LA_REASM_COMPLETE : LA_REASM_IN_PROGRESS;
	} else {
		ret = finfo->is_final_fragment ? LA_REASM_COMPLETE : LA_REASM_IN_PROGRESS;
	}

end:

	// Update fragment counter and expire old entries if necessary.
	// Expiration is performed in relation to rx_time of the fragment currently
	// being processed. This allows processing historical data with timestamps in
	// the past.

	if(++rtable->frag_cnt > rtable->cleanup_interval) {
		la_reasm_table_cleanup(rtable, finfo->rx_time);
		rtable->frag_cnt = 0;
	}
	la_debug_print(D_INFO, "Result: %d\n", ret);
	LA_XFREE(lookup_key);
	return ret;
}

// Returns the reassembled payload and removes the packet data from reassembly table
int la_reasm_payload_get(la_reasm_table *rtable, void const *msg_info, uint8_t **result) {
	la_assert(rtable != NULL);
	la_assert(msg_info != NULL);
	la_assert(result != NULL);

	void *tmp_key = rtable->funcs.get_tmp_key(msg_info);
	la_assert(tmp_key);

	size_t result_len = -1;
	la_reasm_table_entry *rt_entry = la_hash_lookup(rtable->fragment_table, tmp_key);
	if(rt_entry == NULL) {
		result_len = -1;
		goto end;
	}
	if(rt_entry->frags_collected_total_len < 1) {
		result_len = 0;
		goto end;
	}
	// Append a NULL byte at the end of the reassembled buffer, so that it can be
	// cast to char * if this is a text message.
	uint8_t *reasm_buf = LA_XCALLOC(rt_entry->frags_collected_total_len + 1, sizeof(uint8_t));
	uint8_t *ptr = reasm_buf;
	for(la_list *l = rt_entry->fragment_list; l != NULL; l = la_list_next(l)) {
		la_octet_string *ostring = l->data;
		memcpy(ptr, ostring->buf, ostring->len);
		ptr += ostring->len;
	}
	reasm_buf[rt_entry->frags_collected_total_len] = '\0'; // buffer len is frags_collected_total_len + 1
	*result = reasm_buf;
	result_len = rt_entry->frags_collected_total_len;
	la_hash_remove(rtable->fragment_table, tmp_key);
end:
	LA_XFREE(tmp_key);
	return result_len;
}

char const *la_reasm_status_name_get(la_reasm_status status) {
	static char const *reasm_status_names[] = {
		[LA_REASM_UNKNOWN] = "unknown",
		[LA_REASM_COMPLETE] = "complete",
		[LA_REASM_IN_PROGRESS] = "in progress",
		[LA_REASM_SKIPPED] = "skipped",
		[LA_REASM_DUPLICATE] = "duplicate",
		[LA_REASM_FRAG_OUT_OF_SEQUENCE] = "out of sequence",
		[LA_REASM_ARGS_INVALID] = "invalid args"
	};
	if(status < 0 || status > LA_REASM_STATUS_MAX) {
		return NULL;
	}
	return reasm_status_names[status];
}
