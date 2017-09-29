/*
 * ref.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_REF_H_
#define RQL_REF_H_

typedef struct rql_ref_s  rql_ref_t;

#include <inttypes.h>

struct rql_ref_s
{
    uint64_t ref;
};

#endif /* RQL_REF_H_ */
