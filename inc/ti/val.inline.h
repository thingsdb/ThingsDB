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
#include <ti/val.h>
#include <ti/varr.h>
#include <ti/vset.h>

static inline _Bool ti_val_is_object(ti_val_t * val)
{
    return val->tp == TI_VAL_THING && ti_thing_is_object((ti_thing_t *) val);
}

static inline _Bool ti_val_is_instance(ti_val_t * val)
{
    return val->tp == TI_VAL_THING && ti_thing_is_instance((ti_thing_t *) val);
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



