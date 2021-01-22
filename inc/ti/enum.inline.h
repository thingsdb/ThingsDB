/*
 * ti/enum.inline.h
 */
#ifndef TI_ENUM_INLINE_H_
#define TI_ENUM_INLINE_H_

#include <ti/enum.h>
#include <ti/spec.h>
#include <ti/val.h>
#include <tiinc.h>
#include <util/vec.h>

static uint16_t ti_enum_spec_map[] = {
    TI_SPEC_INT,
    TI_SPEC_FLOAT,
    TI_SPEC_STR,
    TI_SPEC_BYTES,
    TI_SPEC_OBJECT,
};

static const char * ti_enum_str_map[] = {
    TI_VAL_INT_S,
    TI_VAL_FLOAT_S,
    TI_VAL_STR_S,
    TI_VAL_BYTES_S,
    TI_VAL_THING_S,
};

static inline ti_val_t * ti_enum_dval(ti_enum_t * enum_)
{
    ti_val_t * val = VEC_first(enum_->members);
    ti_incref(val);
    return val;
}


static inline uint16_t ti_enum_spec(ti_enum_t * enum_)
{
    return ti_enum_spec_map[enum_->enum_tp];
}

static inline const char * ti_enum_tp_str(ti_enum_t * enum_)
{
    return ti_enum_str_map[enum_->enum_tp];
}

static inline ti_member_t * ti_enum_member_by_idx(
        ti_enum_t * enum_,
        uint16_t idx)
{
    return vec_get_or_null(enum_->members, idx);
}

static inline ti_member_t * ti_enum_member_by_strn(
        ti_enum_t * enum_,
        const char * str,
        size_t n)
{
    return smap_getn(enum_->smap, str, n);
}

static inline ti_member_t * ti_enum_member_by_raw(
        ti_enum_t * enum_,
        ti_raw_t * raw)
{
    return smap_getn(enum_->smap, (const char *) raw->data, raw->n);
}

static inline int ti_enum_try_lock(ti_enum_t * enum_, ex_t * e)
{
    if (enum_->flags & TI_ENUM_FLAG_LOCK)
    {
        ex_set(e, EX_OPERATION,
            "cannot change enum `%s` while the enumerator is being used",
            enum_->name);
        return -1;
    }
    return (enum_->flags |= TI_ENUM_FLAG_LOCK) & 0;
}

static inline int ti_enum_ensure_lock(ti_enum_t * enum_)
{
    return (enum_->flags & TI_ENUM_FLAG_LOCK)
            ? 0
            : !!(enum_->flags |= TI_ENUM_FLAG_LOCK);
}

static inline void ti_enum_unlock(ti_enum_t * enum_, int lock_was_set)
{
    if (lock_was_set)
        enum_->flags &= ~TI_ENUM_FLAG_LOCK;
}

#endif  /* TI_ENUM_INLINE_H_ */
