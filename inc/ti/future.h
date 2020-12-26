/*
 * ti/future.h
 */
#ifndef TI_FUTURE_H_
#define TI_FUTURE_H_

typedef struct ti_future_s ti_future_t;

#define VFUT(__v) ((ti_future_t *) (__v))->rval

#include <inttypes.h>
#include <ti/query.t.h>
#include <ti/val.t.h>
#include <ti/addon.h>
#include <util/vec.h>

ti_future_t * ti_future_create(ti_query_t * query, size_t nargs);

void ti_future_destroy(ti_future_t * future);

struct ti_future_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _flags;
    uint16_t _pad1;
    ti_query_t * query;     /* parent query */
    ti_closure_t * then;    /* optional closure, then */
    ti_val_t * rval;        /* resolved value or NULL */
    vec_t * args;           /* arguments */
    ti_addon_t * addon;
};

static inline int ti_future_to_pk(
        ti_future_t * future,
        msgpack_packer * pk,
        int options)
{
    assert (options >= 0);
    return future->rval
            ? ti_val_to_pk(future->rval, pk, options)
            : msgpack_pack_nil(pk);
}

#endif  /* TI_FUTURE_H_ */
