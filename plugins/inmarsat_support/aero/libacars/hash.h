/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */
#ifndef LA_HASH_H
#define LA_HASH_H 1

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct la_hash_s la_hash;
typedef uint32_t (la_hash_func)(void const *key);
typedef bool (la_hash_compare_func)(void const *key1, void const *key2);
typedef bool (la_hash_if_func)(void const *key, void const *value, void *ctx);
typedef void (la_hash_key_destroy_func)(void *key);
typedef void (la_hash_value_destroy_func)(void *value);

#define LA_HASH_INIT 5381
#define LA_HASH_MULTIPLIER 17

la_hash *la_hash_new(la_hash_func *compute_hash, la_hash_compare_func *compare_keys,
		la_hash_key_destroy_func *destroy_key, la_hash_value_destroy_func *destroy_value);
bool la_hash_insert(la_hash *h, void *key, void *value);
bool la_hash_remove(la_hash *h, void *key);
void *la_hash_lookup(la_hash const *h, void const *key);
uint32_t la_hash_key_str(void const *k);
uint32_t la_hash_string(char const *str, uint32_t h);
bool la_hash_compare_keys_str(void const *key1, void const *key2);
void la_simple_free(void *data);
int la_hash_foreach_remove(la_hash *h, la_hash_if_func *if_func, void *if_func_ctx);
void la_hash_destroy(la_hash *h);

#ifdef __cplusplus
}
#endif

#endif // !LA_HASH_H
