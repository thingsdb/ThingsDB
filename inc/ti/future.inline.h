/*
 * ti/future.inline.h
 */
#ifndef TI_FUTURE_INLINE_H_
#define TI_FUTURE_INLINE_H_

#include <ti/future.t.h>
#include <ti/val.inline.h>
#include <ti/closure.inline.h>
#include <ti/vp.t.h>
#include <util/mpack.h>
#include <util/link.h>

static inline int ti_future_to_pk(
        ti_future_t * future,
        ti_vp_t * vp,
        int options)
{
    assert (options >= 0);
    return future->rval
            ? ti_val_to_pk(future->rval, vp, options)
            : msgpack_pack_nil(&vp->pk);
}

static inline int ti_future_register(ti_future_t * future)
{
    int rc = link_insert(&future->query->futures, future);
    if (!rc)
        ti_incref(future);
    return rc;
}

static inline void ti_future_forget_cb(ti_closure_t * cb)
{
    if (!cb)
        return;
    ti_closure_dec_future(cb);
    ti_val_unsafe_drop((ti_val_t *) cb);
}

#endif  /* TI_FUTURE_INLINE_H_ */
