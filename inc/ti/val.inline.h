/*
 * ti/val.inline.h
 */
#ifndef TI_VAL_INLINE_H_
#define TI_VAL_INLINE_H_

#include <ex.h>
#include <ti/closure.h>
#include <ti/collection.h>
#include <ti/name.h>
#include <ti/thing.h>
#include <ti/thing.inline.h>
#include <ti/val.h>
#include <ti/varr.h>
#include <ti/vset.h>

static inline void ti_val_safe_drop(ti_val_t * val)
{
    if (val && !--val->ref)
        ti_val_destroy(val);
}

static inline void ti_val_unsafe_drop(ti_val_t * val)
{
    if (!--val->ref)
        ti_val_destroy(val);
}

static inline void ti_val_gc_drop(ti_val_t * val)
{
    if (!--val->ref)
        ti_val_destroy(val);
    else
        ti_thing_may_push_gc((ti_thing_t *) val);
}

static inline void ti_val_drop(ti_val_t * val)
{
    if (val)
    {
        if (!--val->ref)
            ti_val_destroy(val);
        else
            ti_thing_may_push_gc((ti_thing_t *) val);
    }
}

static inline _Bool ti_val_is_arr(ti_val_t * val)
{
    return val->tp == TI_VAL_ARR;
}

static inline _Bool ti_val_is_bool(ti_val_t * val)
{
    return val->tp == TI_VAL_BOOL;
}

static inline _Bool ti_val_is_closure(ti_val_t * val)
{
    return val->tp == TI_VAL_CLOSURE;
}

static inline _Bool ti_val_is_error(ti_val_t * val)
{
    return val->tp == TI_VAL_ERROR;
}

static inline _Bool ti_val_is_number(ti_val_t * val)
{
    return val->tp == TI_VAL_FLOAT || val->tp == TI_VAL_INT;
}

static inline _Bool ti_val_is_float(ti_val_t * val)
{
    return val->tp == TI_VAL_FLOAT;
}

static inline _Bool ti_val_is_int(ti_val_t * val)
{
    return val->tp == TI_VAL_INT;
}

static inline _Bool ti_val_is_nil(ti_val_t * val)
{
    return val->tp == TI_VAL_NIL;
}

static inline _Bool ti_val_is_str(ti_val_t * val)
{
    return val->tp == TI_VAL_STR || val->tp == TI_VAL_NAME;
}

static inline _Bool ti_val_is_bytes(ti_val_t * val)
{
    return val->tp == TI_VAL_BYTES;
}

static inline _Bool ti_val_is_raw(ti_val_t * val)
{
    return val->tp == TI_VAL_STR ||
           val->tp == TI_VAL_NAME ||
           val->tp == TI_VAL_BYTES ||
           val->tp == TI_VAL_MP;
}

static inline _Bool ti_val_is_regex(ti_val_t * val)
{
    return val->tp == TI_VAL_REGEX;
}

static inline _Bool ti_val_is_set(ti_val_t * val)
{
    return val->tp == TI_VAL_SET;
}

static inline _Bool ti_val_is_thing(ti_val_t * val)
{
    return val->tp == TI_VAL_THING;
}

static inline _Bool ti_val_is_wrap(ti_val_t * val)
{
    return val->tp == TI_VAL_WRAP;
}

static inline _Bool ti_val_is_member(ti_val_t * val)
{
    return val->tp == TI_VAL_MEMBER;
}

static inline _Bool ti_val_is_array(ti_val_t * val)
{
    return val->tp == TI_VAL_ARR;
}

static inline _Bool ti_val_is_list(ti_val_t * val)
{
    return val->tp == TI_VAL_ARR && ti_varr_is_list((ti_varr_t *) val);
}

static inline _Bool ti_val_is_tuple(ti_val_t * val)
{
    return val->tp == TI_VAL_ARR && ti_varr_is_tuple((ti_varr_t *) val);
}

static inline _Bool ti_val_has_len(ti_val_t * val)
{
    return (
        val->tp == TI_VAL_STR ||
        val->tp == TI_VAL_NAME ||
        val->tp == TI_VAL_THING ||
        val->tp == TI_VAL_ARR ||
        val->tp == TI_VAL_SET ||
        val->tp == TI_VAL_BYTES ||
        val->tp == TI_VAL_ERROR
    );
}

static inline _Bool ti_val_overflow_cast(double d)
{
    return !(d >= -VAL__CAST_MAX && d < VAL__CAST_MAX);
}

/*
 * Return 0 when a new lock is set, or -1 if failed and `e` is set.
 *
 * Use `ti_val_try_lock(..)` if you require a lock but no current lock
 * may be set. This is when you make changes, for example with `push`.
 *
 * Call `ti_val_unlock(..)` with `true` as second argument after a successful
 * lock.
 */
static inline int ti_val_try_lock(ti_val_t * val, ex_t * e)
{
    if (val->flags & TI_VFLAG_LOCK)
    {
        ex_set(e, EX_OPERATION_ERROR,
            "cannot change type `%s` while the value is being used",
            ti_val_str(val));
        return -1;
    }
    return (val->flags |= TI_VFLAG_LOCK) & 0;
}

/*
 * Returns `lock_was_set`: 0 if already locked, 1 if a new lock is set
 *
 * Use `ti_val_ensure_lock(..)` if you require a lock but it does not matter
 * if the value is already locked by someone else. The return value can be
 * used with the `ti_val_unlock(..)` function.
 *
 * For example in the `map(..)` function requires a lock but since `map`
 * does not make changes it is no problem if another lock was set.
 */
static inline int ti_val_ensure_lock(ti_val_t * val)
{
    return (val->flags & TI_VFLAG_LOCK)
            ? 0
            : !!(val->flags |= TI_VFLAG_LOCK);
}

static inline void ti_val_unlock(ti_val_t * val, int lock_was_set)
{
    if (lock_was_set)
        val->flags &= ~TI_VFLAG_LOCK;
}

static inline _Bool ti_val_is_object(ti_val_t * val)
{
    return val->tp == TI_VAL_THING && ti_thing_is_object((ti_thing_t *) val);
}

static inline _Bool ti_val_is_instance(ti_val_t * val)
{
    return val->tp == TI_VAL_THING && ti_thing_is_instance((ti_thing_t *) val);
}

static inline void ti_val_attach(
        ti_val_t * val,
        ti_thing_t * parent,
        ti_name_t * name)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_MP:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ERROR:
    case TI_VAL_MEMBER:
    case TI_VAL_CLOSURE:
        return;
    case TI_VAL_ARR:
        ((ti_varr_t *) val)->parent = parent;
        ((ti_varr_t *) val)->name = name;
        return;
    case TI_VAL_SET:
        ((ti_vset_t *) val)->parent = parent;
        ((ti_vset_t *) val)->name = name;
        return;
    case TI_VAL_TEMPLATE:
        break;
    }
    assert(0);
    return;
}

/*
 * although the underlying pointer might point to a new value after calling
 * this function, the `old` pointer value can still be used and has at least
 * one reference left.
 *
 * errors:
 *      - memory allocation (vector / set /closure creation)
 *      - lock/syntax/bad data errors in closure
 */
static inline int ti_val_make_assignable(
        ti_val_t ** val,
        ti_thing_t * parent,
        ti_name_t * name,
        ex_t * e)
{
    switch ((ti_val_enum) (*val)->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_MP:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ERROR:
    case TI_VAL_MEMBER:
        return 0;
    case TI_VAL_ARR:
        if (ti_varr_to_list((ti_varr_t **) val))
        {
            ex_set_mem(e);
            return e->nr;
        }
        ((ti_varr_t *) *val)->parent = parent;
        ((ti_varr_t *) *val)->name = name;
        return 0;
    case TI_VAL_SET:
        if (ti_vset_assign((ti_vset_t **) val))
        {
            ex_set_mem(e);
            return e->nr;
        }
        ((ti_vset_t *) *val)->parent = parent;
        ((ti_vset_t *) *val)->name = name;
        return 0;
    case TI_VAL_CLOSURE:
        return ti_closure_unbound((ti_closure_t * ) *val, e);
    case TI_VAL_TEMPLATE:
        break;
    }
    assert(0);
    return -1;
}

static inline int ti_val_make_variable(ti_val_t ** val, ex_t * e)
{
    switch ((ti_val_enum) (*val)->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_MP:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ERROR:
    case TI_VAL_MEMBER:
        return 0;
    case TI_VAL_ARR:
        if (((ti_varr_t *) *val)->parent && ti_varr_to_list((ti_varr_t **) val))
            ex_set_mem(e);
        return e->nr;
    case TI_VAL_SET:
        if (((ti_vset_t *) *val)->parent && ti_vset_assign((ti_vset_t **) val))
            ex_set_mem(e);
        return e->nr;
    case TI_VAL_CLOSURE:
        return ti_closure_unbound((ti_closure_t * ) *val, e);
    case TI_VAL_TEMPLATE:
        break;
    }
    assert(0);
    return -1;
}

#define TI_VAL_PACK_CASE_IMMUTABLE(val__, pk__, options__) \
    case TI_VAL_NIL: \
        return msgpack_pack_nil(pk__); \
    case TI_VAL_INT: \
        return msgpack_pack_int64(pk__, VINT(val__)); \
    case TI_VAL_FLOAT: \
        return msgpack_pack_double(pk__, VFLOAT(val__)); \
    case TI_VAL_BOOL: \
        return VBOOL(val__) \
                ? msgpack_pack_true(pk__) \
                : msgpack_pack_false(pk__); \
    case TI_VAL_MP: \
    { \
        ti_raw_t * r__ = (ti_raw_t *) val__; \
        return options__ >= 0 \
            ? mp_pack_append(pk__, r__->data, r__->n) \
            : -(msgpack_pack_ext(pk__, r__->n, TI_STR_INFO) || \
                msgpack_pack_ext_body(pk__, r__->data, r__->n) \
            ); \
    } \
    case TI_VAL_NAME: \
    case TI_VAL_STR: \
    { \
        ti_raw_t * r__ = (ti_raw_t *) val__; \
        return mp_pack_strn(pk__, r__->data, r__->n); \
    } \
    case TI_VAL_BYTES: \
    { \
        ti_raw_t * r__ = (ti_raw_t *) val__; \
        return mp_pack_bin(pk__, r__->data, r__->n); \
    } \
   case TI_VAL_REGEX: \
       return ti_regex_to_pk((ti_regex_t *) val__, pk__); \
   case TI_VAL_CLOSURE: \
        return ti_closure_to_pk((ti_closure_t *) val__, pk__); \
    case TI_VAL_ERROR: \
        return ti_verror_to_pk((ti_verror_t *) val__, pk__); \
    case TI_VAL_TEMPLATE: \
        assert (0); return -1;

#endif  /* TI_VAL_INLINE_H_ */



