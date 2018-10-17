/*
 * raw.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <string.h>
#include <stdlib.h>
#include <ti/raw.h>

ti_raw_t * ti_raw_new(const unsigned char * raw, size_t n)
{
    ti_raw_t * r = (ti_raw_t *) malloc(sizeof(ti_raw_t) + n);
    if (!r) return NULL;
    r->n = n;
    memcpy(r->data, raw, n);
    return r;
}

ti_raw_t * ti_raw_dup(const ti_raw_t * raw)
{
    size_t sz = sizeof(ti_raw_t) + raw->n;
    ti_raw_t * r = (ti_raw_t *) malloc(sz);
    if (!r) return NULL;
    memcpy(r, raw, sz);
    return r;
}

char * ti_raw_to_str(const ti_raw_t * raw)
{
    char * str = (char *) malloc(raw->n + 1);
    if (!str) return NULL;
    memcpy(str, raw->data, raw->n);
    str[raw->n] = '\0';
    return str;
}


