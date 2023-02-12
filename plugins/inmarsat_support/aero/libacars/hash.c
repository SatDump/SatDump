/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>                     // strcmp
#include <aero/libacars/macros.h>            // la_assert
#include <aero/libacars/hash.h>              // la_hash
#include <aero/libacars/list.h>              // la_list
#include <aero/libacars/util.h>              // LA_XCALLOC, LA_XFREE

#define LA_HASH_SIZE 173

struct la_hash_s {
	la_hash_func *compute_hash;
	la_hash_compare_func *compare_keys;
	la_hash_key_destroy_func *destroy_key;
	la_hash_value_destroy_func *destroy_value;
	la_list *buckets[LA_HASH_SIZE];
};

typedef struct {
	void *key;
	void *value;
} la_hash_element;

uint32_t la_hash_string(char const *str, uint32_t h) {
	int h_work = (int)h;
	for(char const *p = str; *p != '\0'; p++) {
		h_work = h_work * LA_HASH_MULTIPLIER + (int)(*p);
	}
	return (uint32_t)h_work;
}

uint32_t la_hash_key_str(void const *k) {
	la_assert(k != NULL);
	char const *str = k;
	return la_hash_string(str, LA_HASH_INIT);
}

void la_simple_free(void *data) {
	LA_XFREE(data);
}

static void la_hash_destroy_key(la_hash *h, void *key) {
	la_assert(h != NULL);
	if(h->destroy_key != NULL) {
		h->destroy_key(key);
	}
}

static void la_hash_destroy_value(la_hash *h, void *value) {
	la_assert(h != NULL);
	if(h->destroy_value != NULL) {
		h->destroy_value(value);
	}
}

bool la_hash_compare_keys_str(void const *key1, void const *key2) {
	return (key1 != NULL && key2 != NULL && strcmp(key1, key2) == 0);
}

la_hash *la_hash_new(la_hash_func *compute_hash, la_hash_compare_func *compare_keys,
		la_hash_key_destroy_func *destroy_key, la_hash_value_destroy_func *destroy_value) {

	LA_NEW(la_hash, h);
	h->compute_hash = (compute_hash ? compute_hash : la_hash_key_str);
	h->compare_keys = (compare_keys ? compare_keys : la_hash_compare_keys_str);
	h->destroy_key = destroy_key;        // no default; might be NULL
	h->destroy_value = destroy_value;    // no default; might be NULL
	return h;
}

static size_t la_hash_get_bucket(la_hash const *h, void const *key) {
	return h->compute_hash(key) % LA_HASH_SIZE;
}

static la_list *la_hash_lookup_list_node(la_hash const *h, void const *key,
		la_list **prev_node) {
	la_assert(h != NULL);
	la_assert(key != NULL);

	size_t bucket = la_hash_get_bucket(h, key);
	la_list *l = h->buckets[bucket];
	la_list *pl = NULL;
	la_hash_element *elem;
	for(; l != NULL; pl = l, l = la_list_next(l)) {
		elem = l->data;
		if(h->compare_keys(key, elem->key) == true) {
			break;
		}
	}
	// Give back pointer to the previous node if the caller wants it
	if(prev_node != NULL) {
		*prev_node = pl;
	}
	// Return the node containing the requested key (or NULL if not found)
	return l;
}

void *la_hash_lookup(la_hash const *h, void const *key) {
	la_assert(h != NULL);
	la_assert(key != NULL);

	la_list *list_node = la_hash_lookup_list_node(h, key, NULL);
	if(list_node == NULL) {
		return NULL;
	}
	la_hash_element *elem = list_node->data;
	return (void *)elem->value;
}

// Inserts the new key into the hash and assigns it a value.  Key and value
// pointers are copied directly into the hash, so these variables must exist
// during the entire lifetime of the hash entry (no automatic / static
// variables here).  If the key already exists in the hash, the old value is
// freed, and the key value is replaced with the new one. The key in the hash
// is not replaced, but the key given as argument is freed in this case.
bool la_hash_insert(la_hash *h, void *key, void *value) {
	la_assert(h != NULL);
	la_assert(key != NULL);

	la_list *list_node = la_hash_lookup_list_node(h, key, NULL);
	if(list_node != NULL) {
		// Key already exists - insert the new value, free the old value,
		// preserve the old key, free the new key
		la_hash_element *elem = list_node->data;
		la_hash_destroy_key(h, key);
		la_hash_destroy_value(h, elem->value);
		elem->value = value;
		return true;
	}
	// Key not found - create new hash entry
	LA_NEW(la_hash_element, elem);
	elem->key = key;
	elem->value = value;
	size_t bucket = la_hash_get_bucket(h, key);
	h->buckets[bucket] = la_list_append(h->buckets[bucket], elem);
	return false;
}

// Removes the key from the hash. Frees both key and value which were
// stored in the hash. The key given as an argument is not freed,
// so a static value is allowed here.
bool la_hash_remove(la_hash *h, void *key) {
	la_assert(h != NULL);
	la_assert(key != NULL);

	la_list *prev_node = NULL;
	la_list *list_node = la_hash_lookup_list_node(h, key, &prev_node);
	if(list_node == NULL) {
		return false;
	}
	// If prev_node is not NULL, then join two list parts together.
	// Otherwise replace the top of the list in the bucket.
	if(prev_node != NULL) {
		prev_node->next = list_node->next;
	} else {
		size_t bucket = la_hash_get_bucket(h, key);
		h->buckets[bucket] = list_node->next;
	}
	list_node->next = NULL;
	// Now free the key and value
	la_hash_element *elem = list_node->data;
	la_hash_destroy_key(h, elem->key);
	la_hash_destroy_value(h, elem->value);
	// This will free elem too
	// XXX: use la_hash_element_destroy instead?
	la_list_free(list_node);
	return true;
}

static void la_free_noop(void *data) {
	LA_UNUSED(data);
	// noop
}

// Iterates over hash entries executing la_hash_if_func() for each key-value
// pair. If the func returns true, removes the entry from the hash.
int la_hash_foreach_remove(la_hash *h, la_hash_if_func *if_func, void *if_func_ctx) {
	la_assert(h != NULL);
	la_assert(if_func != NULL);

	la_list *keys_to_delete = NULL;
	int num_keys_deleted = 0;
	for(size_t i = 0; i < LA_HASH_SIZE; i++) {
		for(la_list *l = h->buckets[i]; l != NULL; l = la_list_next(l)) {
			la_hash_element *elem = l->data;
			if(if_func(elem->key, elem->value, if_func_ctx) == true) {
				keys_to_delete = la_list_append(keys_to_delete, elem->key);
				num_keys_deleted++;
			}
		}
	}
	for(la_list *l = keys_to_delete; l != NULL; l = la_list_next(l)) {
		la_hash_remove(h, l->data);
	}
	// can't use la_list_free() here, as keys have already been freed by la_hash_remove()
	la_list_free_full(keys_to_delete, la_free_noop);
	return num_keys_deleted;
}

// This is a callback for la_list_free_full_with_ctx()
static void la_hash_element_destroy(void *elemptr, void *hashptr) {
	la_assert(hashptr != NULL);
	if(elemptr == NULL) {
		return;
	}
	la_hash *h = hashptr;
	la_hash_element *elem = elemptr;
	la_hash_destroy_key(h, elem->key);
	la_hash_destroy_value(h, elem->value);
	LA_XFREE(elem);
}

// Deallocates all keys and values (if destroy functions have been provided)
// and then frees the memory used by the hash.
void la_hash_destroy(la_hash *h) {
	if(h == NULL) {
		return;
	}
	for(size_t i = 0; i < LA_HASH_SIZE; i++) {
		la_list_free_full_with_ctx(h->buckets[i], la_hash_element_destroy, h);
		h->buckets[i] = NULL;
	}
	LA_XFREE(h);
}
