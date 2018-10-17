/*
 * raw.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef TI_RAW_H_
#define TI_RAW_H_

#include <qpack.h>

typedef qp_raw_t ti_raw_t;

#include <stddef.h>

ti_raw_t * ti_raw_new(const unsigned char * raw, size_t n);
ti_raw_t * ti_raw_dup(const ti_raw_t * raw);
char * ti_raw_to_str(const ti_raw_t * raw);
static inline _Bool ti_raw_equal(const ti_raw_t * a, const ti_raw_t * b);


static inline _Bool ti_raw_equal(const ti_raw_t * a, const ti_raw_t * b)
{
    return a->n == b->n && !memcmp(a->data, b->data, a->n);
}

#endif /* TI_RAW_H_ */
