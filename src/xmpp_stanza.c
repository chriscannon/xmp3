/**
 * xmp3 - XMPP Proxy
 * xmpp_stanza.{c,h} - Represents top-level XMPP stanzas received from clients.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include <stdlib.h>

#include <uthash.h>
#include <utlist.h>
#include <utstring.h>

#include "log.h"

#include "xmpp_parser.h"

#include "xmpp_stanza.h"

struct attribute {
    char *name;
    char *value;

    /** These are stored in a hash table. */
    UT_hash_handle hh;
};

struct xmpp_stanza {
    /** The namespace of this tag. */
    char *namespace;

    /** The name of this tag. */
    char *name;

    /** A hash table of stanza attributes. */
    struct attribute *attributes;

    /** The data inside this node */
    UT_string data;

    /** A linked list of child nodes. */
    struct xmpp_stanza *children;

    struct xmpp_stanza *parent;

    /** Stanzas are kept in a linked list. */
    struct xmpp_stanza *prev;
    struct xmpp_stanza *next;
};

struct xmpp_stanza* xmpp_stanza_new(const char *ns_name, const char **attrs) {
    struct xmpp_stanza *stanza = calloc(1, sizeof(*stanza));
    check_mem(stanza);

    char *separator = strchr(ns_name, XMPP_PARSER_SEPARATOR);
    if (separator != NULL) {
        STRNDUP_CHECK(stanza->namespace, ns_name, separator - ns_name);
        STRDUP_CHECK(stanza->name, separator + 1);
    } else {
        STRDUP_CHECK(stanza->name, name);
    }

    utstring_init(&stanza->data);

    for (int i = 0; attrs[i] != NULL; i += 2) {
        struct attribute *attr = calloc(1, sizeof(*attr));
        check_mem(attr);
        STRDUP_CHECK(attr->name, attrs[i]);
        STRDUP_CHECK(attr->value, attrs[i + 1]);
        HASH_ADD_KEYPTR(hh, stanza->attributes, attr->name, strlen(attr->name),
                        attr);
    }
    return stanza;
}

void xmpp_stanza_del(struct xmpp_stanza *stanza, bool recursive) {
    free(stanza->namespace);
    free(stanza->name);

    struct attribute *attr, *tmp;
    HASH_ITER(hh, stanza->attributes, attr, tmp) {
        HASH_DEL(stanza->attributes, attr);
        free(attr);
    }

    utstring_done(&stanza->data);

    if (recursive) {
        struct xmpp_stanza *s, *tmp;
        DL_FOREACH_SAFE(stanza->children, s, tmp) {
            DL_DELETE(stanza->children, s);
            xmpp_stanza_del(s);
        }
    }

    free(stanza);
}

const char* xmpp_stanza_namespace(const struct xmpp_stanza *stanza) {
    return stanza->namespace;
}

void xmpp_stanza_copy_namespace(struct xmpp_stanza *stanza, const char *ns) {
    if (stanza->namespace) {
        free(stanza->namespace);
    }
    STRDUP_CHECK(stanza->namespace, ns);
}

const char* xmpp_stanza_name(const struct xmpp_stanza *stanza) {
    return stanza->name;
}

void xmpp_stanza_copy_name(const struct xmpp_stanza *stanza,
                           const char *name) {
    if (stanza->name) {
        free(stanza->name);
    }
    STRDUP_CHECK(stanza->name, name);
}

const char* xmpp_stanza_attr(const struct xmpp_stanza *stanza,
                             const char *name) {
    struct attribute *attr;
    HASH_FIND_STR(stanza->attributes, name, attr);
    if (attr == NULL) {
        return NULL;
    } else {
        return attr->value;
    }
}

void xmpp_stanza_set_attr(struct xmpp_stanza *stanza, const char *name,
                          const char *value) {
    struct attribute *attr;
    HASH_FIND_STR(stanza->attributes, name, attr);

    if (attr == NULL) {
        if (value == NULL) {
            return;
        }
        attr = calloc(1, sizeof(*attr));
        check_mem(attr);
        attr->name = strdup(name);
        check_mem(attr->name);
        HASH_ADD_KEYPTR(hh, stanza->attributes, name, strlen(name), attr);
    } else {
        if (value == NULL) {
            HASH_DEL(stanza->attributes, attr);
            free(attr);
            return;
        }
        free(attr->value);
    }
    attr->value = strdup(value);
    check_mem(attr->value);
}

const char* xmpp_stanza_data(const struct xmpp_stanza *stanza) {
    return utstring_body(stanza->data);
}

int xmpp_stanza_data_length(const struct xmpp_stanza *stanza) {
    return utstring_len(stanza->data);
}

void xmpp_stanza_append_data(struct xmpp_stanza *stanza, const char *buf,
                             int len) {
    utstring_bincopy(stanza->data, buf, len);
}

int xmpp_stanza_children_length(const struct xmpp_stanza *stanza) {
    int count = 0;
    struct xmpp_stanza *s;
    DL_FOREACH(stanza->children, s) {
        count++;
    }
    return count;
}

struct xmpp_stanza* xmpp_stanza_children(struct xmpp_stanza *stanza) {
    return stanza->children;
}

struct xmpp_stanza* xmpp_stanza_parent(struct xmpp_stanza *stanza) {
    return stanza->parent;
}

struct xmpp_stanza* xmpp_stanza_next(struct xmpp_stanza *stanza) {
    return stanza->next;
}

struct xmpp_stanza* xmpp_stanza_prev(struct xmpp_stanza *stanza) {
    return stanza->prev;
}

void xmpp_stanza_append_child(struct xmpp_stanza *stanza,
                              struct xmpp_stanza *child) {
    DL_APPEND(stanza->children, child);
    child->parent = stanza;
}
