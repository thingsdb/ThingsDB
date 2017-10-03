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

static inline void rql_ref_inc(rql_ref_t * ref);
static inline void rql_ref_dec(rql_ref_t * ref);

static inline void rql_ref_inc(rql_ref_t * ref)
{
    ref->ref++;
}

static inline void rql_ref_dec(rql_ref_t * ref)
{
    assert(ref->ref > 1);
    ref->ref--;
}

#endif /* RQL_REF_H_ */
