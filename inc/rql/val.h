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
    RQL_VAL_ARR,
} rql_val_e;
typedef struct rql_val_s rql_val_t;
typedef union rql_val_u rql_val_via_t;

#include <inttypes.h>
#include <rql/raw.h>
#include <rql/elem.h>
#include <util/vec.h>

void rql_val_init(rql_val_t * val, rql_val_e tp, void * v);
void rql_val_clear(rql_val_t * val);

union rql_val_u
{
    rql_elem_t * elem_;
    int64_t int_;
    double float_;
    _Bool bool_;
    char * str_;
    rql_raw_t * raw_;
    vec_t * arr_;  /* not specified by type */
};

struct rql_val_s
{
    rql_val_e tp;
    rql_val_via_t via;
};



#endif /* RQL_VAL_H_ */
