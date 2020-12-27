/*
 * ti/future.inline.h
 */
#ifndef TI_FUTURE_INLINE_H_
#define TI_FUTURE_INLINE_H_

#include <ti/future.t.h>
#include <ti/val.inline.h>
#include <util/mpack.h>
#include <util/link.h>

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

static inline int ti_future_regiter(ti_future_t * future)
{
    int rc = link_insert(&future->query->futures, future);
    if (!rc)
        ti_incref(future);
    return rc;
}

#endif  /* TI_FUTURE_H_ */
