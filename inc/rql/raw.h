/*
 * raw.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_RAW_H_
#define RQL_RAW_H_

#include <qpack.h>

typedef qp_raw_t rql_raw_t;

#include <stddef.h>

rql_raw_t * rql_raw_new(const unsigned char * raw, size_t n);
rql_raw_t * rql_raw_dup(const rql_raw_t * raw);
char * rql_raw_to_str(const rql_raw_t * raw);
static inline _Bool rql_raw_equal(const rql_raw_t * a, const rql_raw_t * b);


static inline _Bool rql_raw_equal(const rql_raw_t * a, const rql_raw_t * b)
{
    return a->n == b->n && !memcmp(a->data, b->data, a->n);
}

#endif /* RQL_RAW_H_ */
