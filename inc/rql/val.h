/*
 * val.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_VAL_H_
#define RQL_VAL_H_

typedef enum
{
    RQL_VAL_ELEM,
    RQL_VAL_INT,
    RQL_VAL_FLOAT,
    RQL_VAL_BOOL,
    RQL_VAL_STR,
    RQL_VAL_RAW,
    RQL_VAL_ELEMS,
    RQL_VAL_PRIMITIVES,
    RQl_VAL_NIL,
} rql_val_e;
typedef struct rql_val_s rql_val_t;
typedef union rql_val_u rql_val_via_t;

#include <inttypes.h>
#include <rql/raw.h>
#include <rql/elem.h>
#include <util/vec.h>

rql_val_t * rql_val_create(rql_val_e tp, void * v);
rql_val_t * rql_val_weak_create(rql_val_e tp, void * v);
void rql_val_destroy(rql_val_t * val);
void rql_val_weak_set(rql_val_t * val, rql_val_e tp, void * v);
int rql_val_set(rql_val_t * val, rql_val_e tp, void * v);
void rql_val_clear(rql_val_t * val);

union rql_val_u
{
    rql_elem_t * elem_;
    int64_t int_;
    double float_;
    _Bool bool_;
    char * str_;
    rql_raw_t * raw_;
    vec_t * elems_;
    vec_t * primitives_;
    void * nil_;
};

struct rql_val_s
{
    rql_val_e tp;
    rql_val_via_t via;
};

#endif /* RQL_VAL_H_ */
