/*
 * ti/wrap.inline.h
 */
#ifndef TI_WRAP_INLINE_H_
#define TI_WRAP_INLINE_H_

#include <ti/wrap.h>
#include <ti/thing.inline.h>

static inline const char * ti_wrap_str(ti_wrap_t * wrap)
{
    ti_type_t * type = imap_get(
            wrap->thing->collection->types->imap,
            wrap->type_id);
    return type ? type->wname : "<thing>";
}

static inline int ti_wrap_to_file(ti_wrap_t * wrap, FILE * f)
{
    return (
            qp_fadd_type(f, QP_MAP1) ||
            qp_fadd_raw(f, (const uchar *) TI_KIND_S_WRAP, 1) ||
            qp_fadd_type(f, QP_ARRAY2) ||
            qp_fadd_int(f, wrap->type_id) ||
            qp_fadd_type(f, QP_MAP1) ||
            qp_fadd_raw(f, (const uchar *) TI_KIND_S_THING, 1) ||
            qp_fadd_int(f, wrap->thing->id)
    );
}

static inline ti_wrap_maybe_type(ti_wrap_t * wrap)
{
    ti_type_t * type = imap_get(
            wrap->thing->collection->types->imap,
            wrap->type_id);
    return type;
}

#endif  /* TI_WRAP_INLINE_H_ */
