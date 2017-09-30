/*
 * val.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_VAL_H_
#define RQL_VAL_H_

typedef union rql_val_u rql_val_via_t;
typedef enum rql_val_e
{
    RQL_VAL_ELEM,
    RQL_VAL_INT,
    RQL_VAL_UINT,
    RQL_VAL_FLOAT,
    RQL_VAL_BOOL,
    RQL_VAL_STR,
    RQL_VAL_RAW
} rql_val_t;

#include <inttypes.h>
#include <vec/vec.h>
#include <rql/raw.h>
#include <rql/elem.h>

union rql_val_u
{
    rql_elem_t * elem_;
    int64_t int_;
    uint64_t uint_;
    double float_;
    _Bool bool_;
    char * str_;
    rql_raw_t * raw_;
    vec_t * arr_;  /* not specified by type */
};

rql_val_via_t * rql_val_create(rql_val_t tp, void * data);
void rql_val_destroy(rql_val_t tp, rql_val_via_t * val);

#endif /* RQL_VAL_H_ */
