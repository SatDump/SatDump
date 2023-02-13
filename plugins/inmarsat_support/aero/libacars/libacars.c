/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */
#include <stdbool.h>
#ifdef DEBUG
#include <stdlib.h>                 // getenv, strtoul
#endif
#include <aero/libacars/macros.h>        // la_assert
#include <aero/libacars/libacars.h>      // la_proto_node
#include <aero/libacars/vstring.h>
#include <aero/libacars/json.h>
#include <aero/libacars/util.h>          // LA_XCALLOC, LA_XFREE

static void la_proto_node_format_text(la_vstring *vstr, la_proto_node const *node, int indent) {
	la_assert(indent >= 0);
	if(node->data != NULL) {
		la_assert(node->td);
		node->td->format_text(vstr, node->data, indent);
	}
	if(node->next != NULL) {
		la_proto_node_format_text(vstr, node->next, indent+1);
	}
}

static void la_proto_node_format_json(la_vstring *vstr, la_proto_node const *node) {
	if(node->td != NULL) {
		if(node->td->json_key != NULL) {
			la_json_object_start(vstr, node->td->json_key);
			// Missing JSON handler for a node is not fatal.
			// In this case an empty JSON object is produced.
			if(node->data != NULL && node->td->format_json != NULL) {
				node->td->format_json(vstr, node->data);
			}
		}
	}
	if(node->next != NULL) {
		la_proto_node_format_json(vstr, node->next);
	}
	if(node->td != NULL && node->td->json_key != NULL) {
		// We've started a JSON object above, so it needs to be closed
		la_json_object_end(vstr);
	}
}

la_proto_node *la_proto_node_new() {
	LA_NEW(la_proto_node, node);
	return node;
}

la_vstring *la_proto_tree_format_text(la_vstring *vstr, la_proto_node const *root) {
	la_assert(root);

	if(vstr == NULL) {
		vstr = la_vstring_new();
	}
	la_proto_node_format_text(vstr, root, 0);
	return vstr;
}

la_vstring *la_proto_tree_format_json(la_vstring *vstr, la_proto_node const *root) {
	la_assert(root);

	if(vstr == NULL) {
		vstr = la_vstring_new();
	}
	la_json_start(vstr);
	la_proto_node_format_json(vstr, root);
	la_json_end(vstr);
	return vstr;
}

void la_proto_tree_destroy(la_proto_node *root) {
	if(root == NULL) {
		return;
	}
	if(root->next != NULL) {
		la_proto_tree_destroy(root->next);
	}
	if(root->td != NULL && root->td->destroy != NULL) {
		root->td->destroy(root->data);
	} else {
		LA_XFREE(root->data);
	}
	LA_XFREE(root);
}

la_proto_node *la_proto_tree_find_protocol(la_proto_node *root, la_type_descriptor const *td) {
	while(root != NULL) {
		if(root->td == td) {
			return root;
		}
		root = root->next;
	}
	return NULL;
}

#ifdef DEBUG

uint32_t Debug = 0;

void __attribute__ ((constructor)) la_debug_init() {
	char *la_debug = getenv("LA_DEBUG");
	if(la_debug != NULL) {
		Debug = strtoul(la_debug, NULL, 10);
	}
}

#endif

#ifdef _WIN32
char * strsep(char **sp, char *sep)
{
    char *p, *s;
    if (sp == NULL || *sp == NULL || **sp == '\0') return(NULL);
    s = *sp;
    p = s + strcspn(s, sep);
    if (*p != '\0') *p++ = '\0';
    *sp = p;
    return(s);
}
#endif