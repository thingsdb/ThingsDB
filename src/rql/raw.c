/*
 * raw.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <string.h>
#include <stdlib.h>
#include <rql/raw.h>

rql_raw_t * rql_raw_new(const unsigned char * raw, size_t n)
{
    rql_raw_t * r = (rql_raw_t *) malloc(sizeof(rql_raw_t) + n);
    if (!r) return NULL;
    r->n = n;
    memcpy(r->data, raw, n);
    return r;
}

rql_raw_t * rql_raw_dup(rql_raw_t * raw)
{
    size_t sz = sizeof(rql_raw_t) + raw->n;
    rql_raw_t * r = (rql_raw_t *) malloc(sz);
    if (!r) return NULL;
    memcpy(r, raw, sz);
    return r;
}
