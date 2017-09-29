/*
 * raw.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <string.h>
#include <stdlib.h>
#include <rql/raw.h>

rql_raw_t * rql_raw_create(const unsigned char * raw, size_t n)
{
    rql_raw_t * raw_ = (rql_raw_t *) malloc(sizeof(rql_raw_t) + n);
    memcpy(raw_->raw, raw, n);
    return raw_;
}

void rql_raw_destroy(rql_raw_t * raw)
{
    free(raw);
}
