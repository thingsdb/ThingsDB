/*
 * raw.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_RAW_H_
#define RQL_RAW_H_

typedef struct rql_raw_s  rql_raw_t;

#include <stddef.h>

struct rql_raw_s
{
    size_t n;
    unsigned char raw[];
};

rql_raw_t * rql_raw_new(const unsigned char * raw, size_t n);

#endif /* RQL_RAW_H_ */
