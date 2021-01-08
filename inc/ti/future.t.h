/*
 * ti/future.t.h
 */
#ifndef TI_FUTURE_T_H_
#define TI_FUTURE_T_H_

typedef struct ti_future_s ti_future_t;

#define VFUT(__v) ((ti_future_t *) (__v))->rval

#include <inttypes.h>
#include <ti/ext.h>
#include <ti/query.t.h>
#include <ti/val.t.h>
#include <ti/closure.t.h>
#include <util/vec.h>

struct ti_future_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _flags;
    uint16_t _pad1;
    ti_query_t * query;     /* parent query */
    ti_closure_t * then;    /* optional closure, then */
    ti_closure_t * fail;    /* optional closure, fail */
    ti_val_t * rval;        /* resolved value or NULL */
    vec_t * args;
    ti_ext_t * ext;
};

#endif  /* TI_FUTURE_T_H_ */
