/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */
#ifndef LA_DICT_H
#define LA_DICT_H 1

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int id;
	void *val;
} la_dict;

void *la_dict_search(la_dict const *list, int id);

#ifdef __cplusplus
}
#endif

#endif // !LA_DICT_H
