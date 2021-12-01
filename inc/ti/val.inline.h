/*
 * ti/val.inline.h
 */
#ifndef TI_VAL_INLINE_H_
#define TI_VAL_INLINE_H_

#include <ex.h>
#include <ti/closure.h>
#include <ti/collection.h>
#include <ti/datetime.h>
#include <ti/future.h>
#include <ti/member.h>
#include <ti/member.inline.h>
#include <ti/name.h>
#include <ti/nil.h>
#include <ti/regex.h>
#include <ti/room.h>
#include <ti/room.inline.h>
#include <ti/str.h>
#include <ti/template.h>
#include <ti/thing.h>
#include <ti/thing.inline.h>
#include <ti/val.h>
#include <ti/varr.inline.h>
#include <ti/vbool.h>
#include <ti/vint.h>
#include <ti/vfloat.h>
#include <ti/vset.h>
#include <ti/vtask.h>
#include <ti/wrap.h>
#include <ti/wrap.inline.h>
#include <util/strx.h>

static inline int val__str_to_str(ti_val_t ** UNUSED(v), ex_t * UNUSED(e));
static inline int val__no_to_str(ti_val_t ** val, ex_t * e);

static inline const char * val__nil_type_str(ti_val_t * UNUSED(val))
{
    return TI_VAL_NIL_S;
}
static inline const char * val__int_type_str(ti_val_t * UNUSED(val))
{
    return TI_VAL_INT_S;
}
static inline const char * val__float_type_str(ti_val_t * UNUSED(val))
{
    return TI_VAL_FLOAT_S;
}
static inline const char * val__bool_type_str(ti_val_t * UNUSED(val))
{
    return TI_VAL_BOOL_S;
}
static inline const char * val__datetime_type_str(ti_val_t * val)
{
    return ti_datetime_is_timeval((ti_datetime_t *) val)
            ? TI_VAL_TIMEVAL_S
            : TI_VAL_DATETIME_S;
}
static inline const char * val__str_type_str(ti_val_t * UNUSED(val))
{
    return TI_VAL_STR_S;
}
static inline const char * val__bytes_type_str(ti_val_t * UNUSED(val))
{
    return TI_VAL_BYTES_S;
}
static inline const char * val__regex_type_str(ti_val_t * UNUSED(val))
{
    return TI_VAL_REGEX_S;
}
static inline const char * val__thing_type_str(ti_val_t * val)
{
    return ti_thing_is_object((ti_thing_t *) val)
            ? TI_VAL_THING_S
            : ti_thing_type_str((ti_thing_t *) val);
}
static inline const char * val__wrap_type_str(ti_val_t * val)
{
    return ti_wrap_type_str((ti_wrap_t *) val);
}
static inline const char * val__room_type_str(ti_val_t * UNUSED(val))
{
    return TI_VAL_ROOM_S;
}
static inline const char * val__task_type_str(ti_val_t * UNUSED(val))
{
    return TI_VAL_TASK_S;
}
static inline const char * val__arr_type_str(ti_val_t * val)
{
    return ti_varr_is_list((ti_varr_t *) val)
            ? TI_VAL_LIST_S
            : TI_VAL_TUPLE_S;
}
static inline const char * val__set_type_str(ti_val_t * UNUSED(val))
{
    return TI_VAL_SET_S;
}
static inline const char * val__error_type_str(ti_val_t * UNUSED(val))
{
    return TI_VAL_ERROR_S;
}
static inline const char * val__member_type_str(ti_val_t * val)
{
    return ti_member_enum_name((ti_member_t *) val);
}
static inline const char * val__mpdata_type_str(ti_val_t * UNUSED(val))
{
    return TI_VAL_MPDATA_S;
}
static inline const char * val__closure_type_str(ti_val_t * UNUSED(val))
{
    return TI_VAL_CLOSURE_S;
}
static inline const char * val__future_type_str(ti_val_t * UNUSED(val))
{
    return TI_VAL_FUTURE_S;
}

static inline int val__nil_to_pk(ti_val_t * UNUSED(v), ti_vp_t * vp, int UNUSED(o))
{
    return msgpack_pack_nil(&vp->pk);
}
static inline int val__int_to_pk(ti_val_t * val, ti_vp_t * vp, int UNUSED(o))
{
    return msgpack_pack_int64(&vp->pk, VINT(val));
}
static inline int val__float_to_pk(ti_val_t * val, ti_vp_t * vp, int UNUSED(o))
{
    return msgpack_pack_double(&vp->pk, VFLOAT(val));
}
static inline int val__bool_to_pk(ti_val_t * val, ti_vp_t * vp, int UNUSED(o))
{
    return ti_vbool_to_pk((ti_vbool_t *) val, &vp->pk);
}
static inline int val__datetime_to_pk(ti_val_t * val, ti_vp_t * vp, int options)
{
    return ti_datetime_to_pk((ti_datetime_t *) val, &vp->pk, options);
}
static inline int val__str_to_pk(ti_val_t * val, ti_vp_t * vp, int UNUSED(o))
{
    return ti_raw_str_to_pk((ti_raw_t *) val, &vp->pk);
}
static inline int val__bytes_to_pk(ti_val_t * val, ti_vp_t * vp, int UNUSED(o))
{
    return ti_raw_bytes_to_pk((ti_raw_t *) val, &vp->pk);
}
static inline int val__regex_to_pk(ti_val_t * val, ti_vp_t * vp, int options)
{
    return ti_regex_to_pk((ti_regex_t *) val, &vp->pk, options);
}
static inline int val__room_to_pk(ti_val_t * val, ti_vp_t * vp, int options)
{
    return ti_room_to_pk((ti_room_t *) val, &vp->pk, options);
}
static inline int val__task_to_pk(ti_val_t * val, ti_vp_t * vp, int options)
{
    return ti_vtask_to_pk((ti_vtask_t *) val, &vp->pk, options);
}
static inline int val__error_to_pk(ti_val_t * val, ti_vp_t * vp, int options)
{
    return ti_verror_to_pk((ti_verror_t *) val, &vp->pk, options);
}
static inline int val__mpdata_to_pk(ti_val_t * val, ti_vp_t * vp, int options)
{
    return ti_raw_mpdata_to_pk((ti_raw_t *) val, &vp->pk, options);
}
static inline int val__closure_to_pk(ti_val_t * val, ti_vp_t * vp, int options)
{
    return ti_closure_to_pk((ti_closure_t *) val, &vp->pk, options);
}
static inline int val__future_to_pk(ti_future_t * future, ti_vp_t * vp, int options);
static inline int val__member_to_pk(ti_member_t * member, ti_vp_t * vp, int options);
static inline int val__varr_to_pk(ti_varr_t * varr, ti_vp_t * vp, int options);

typedef void (*ti_val_destroy_cb) (ti_val_t *);
typedef int (*ti_val_to_str_cb) (ti_val_t **, ex_t *);
typedef int (*ti_val_to_pk_cb) (ti_val_t *, ti_vp_t *, int);
typedef const char * (*ti_val_type_str_cb) (ti_val_t *);

typedef struct
{
    ti_val_destroy_cb destroy;
    ti_val_to_str_cb to_str;
    ti_val_to_pk_cb to_pk;
    ti_val_type_str_cb get_type_str;
    _Bool allowed_as_vtask_arg;     /* allowed in the @thingsdb scope */
} ti_val_type_t;


static ti_val_type_t ti_val_type_props[21] = {
    /* TI_VAL_NIL */
    {
        .destroy = (ti_val_destroy_cb) free,
        .to_str = ti_val_nil_to_str,
        .to_pk = val__nil_to_pk,
        .get_type_str = val__nil_type_str,
        .allowed_as_vtask_arg = true,
    },
    /* TI_VAL_INT */
    {
        .destroy = (ti_val_destroy_cb) free,
        .to_str = ti_val_int_to_str,
        .to_pk = val__int_to_pk,
        .get_type_str = val__int_type_str,
        .allowed_as_vtask_arg = true,
    },
    /* TI_VAL_FLOAT */
    {
        .destroy = (ti_val_destroy_cb) free,
        .to_str = ti_val_float_to_str,
        .to_pk = val__float_to_pk,
        .get_type_str = val__float_type_str,
        .allowed_as_vtask_arg = true,
    },
    /* TI_VAL_BOOL */
    {
        .destroy = (ti_val_destroy_cb) free,
        .to_str = ti_val_bool_to_str,
        .to_pk = val__bool_to_pk,
        .get_type_str = val__bool_type_str,
        .allowed_as_vtask_arg = true,
    },
    /* TI_VAL_DATETIME */
    {
        .destroy = (ti_val_destroy_cb) free,
        .to_str = ti_val_datetime_to_str,
        .to_pk = val__datetime_to_pk,
        .get_type_str = val__datetime_type_str,
        .allowed_as_vtask_arg = true,
    },
    /* TI_VAL_NAME */
    {
        .destroy = (ti_val_destroy_cb) ti_name_destroy,
        .to_str = val__str_to_str,
        .to_pk = val__str_to_pk,
        .get_type_str = val__str_type_str,
        .allowed_as_vtask_arg = true,
    },
    /* TI_VAL_STR */
    {
        .destroy = (ti_val_destroy_cb) free,
        .to_str = val__str_to_str,
        .to_pk = val__str_to_pk,
        .get_type_str = val__str_type_str,
        .allowed_as_vtask_arg = true,
    },
    /* TI_VAL_BYTES */
    {
        .destroy = (ti_val_destroy_cb) free,
        .to_str = ti_val_bytes_to_str,
        .to_pk = val__bytes_to_pk,
        .get_type_str = val__bytes_type_str,
        .allowed_as_vtask_arg = true,
    },
    /* TI_VAL_REGEX */
    {
        .destroy = (ti_val_destroy_cb) ti_regex_destroy,
        .to_str = ti_val_regex_to_str,
        .to_pk = val__regex_to_pk,
        .get_type_str = val__regex_type_str,
        .allowed_as_vtask_arg = true,
    },
    /* TI_VAL_THING */
    {
        .destroy = (ti_val_destroy_cb) ti_thing_destroy,
        .to_str = ti_val_thing_to_str,
        .to_pk = (ti_val_to_pk_cb) ti_thing_to_pk,
        .get_type_str = val__thing_type_str,
        .allowed_as_vtask_arg = false,
    },
    /* TI_VAL_WRAP */
    {
        .destroy = (ti_val_destroy_cb) ti_wrap_destroy,
        .to_str = ti_val_wrap_to_str,
        .to_pk = (ti_val_to_pk_cb) ti_wrap_to_pk,
        .get_type_str = val__wrap_type_str,
        .allowed_as_vtask_arg = false,
    },
    /* TI_VAL_ROOM */
    {
        .destroy = (ti_val_destroy_cb) ti_room_destroy,
        .to_str = ti_val_room_to_str,
        .to_pk = val__room_to_pk,
        .get_type_str = val__room_type_str,
        .allowed_as_vtask_arg = false,
    },
    /* TI_VAL_TASK */
    {
        .destroy = (ti_val_destroy_cb) ti_vtask_destroy,
        .to_str = ti_val_vtask_to_str,
        .to_pk = val__task_to_pk,
        .get_type_str = val__task_type_str,
        .allowed_as_vtask_arg = false,
    },
    /* TI_VAL_ARR */
    {
        .destroy = (ti_val_destroy_cb) ti_varr_destroy,
        .to_str = val__no_to_str,
        .to_pk = (ti_val_to_pk_cb) val__varr_to_pk,
        .get_type_str = val__arr_type_str,
        .allowed_as_vtask_arg = false,
    },
    /* TI_VAL_SET */
    {
        .destroy = (ti_val_destroy_cb) ti_vset_destroy,
        .to_str = val__no_to_str,
        .to_pk = (ti_val_to_pk_cb) ti_vset_to_pk,
        .get_type_str = val__set_type_str,
        .allowed_as_vtask_arg = false,
    },
    /* TI_VAL_ERROR */
    {
        .destroy = (ti_val_destroy_cb) free,
        .to_str = ti_val_error_to_str,
        .to_pk = val__error_to_pk,
        .get_type_str = val__error_type_str,
        .allowed_as_vtask_arg = false,
    },
    /* TI_VAL_MEMBER */
    {
        .destroy = (ti_val_destroy_cb) ti_member_destroy,
        .to_str = ti_val_member_to_str,
        .to_pk = (ti_val_to_pk_cb) val__member_to_pk,
        .get_type_str = val__member_type_str,
        .allowed_as_vtask_arg = false,
    },
    /* TI_VAL_MPDATA */
    {
        .destroy = (ti_val_destroy_cb) free,
        .to_str = val__no_to_str,
        .to_pk = val__mpdata_to_pk,
        .get_type_str = val__mpdata_type_str,
        .allowed_as_vtask_arg = false,
    },
    /* TI_VAL_CLOSURE */
    {
        .destroy = (ti_val_destroy_cb) ti_closure_destroy,
        .to_str = ti_val_closure_to_str,
        .to_pk = val__closure_to_pk,
        .get_type_str = val__closure_type_str,
        .allowed_as_vtask_arg = false,
    },
    /* TI_VAL_FUTURE */
    {
        .destroy = (ti_val_destroy_cb) ti_future_destroy,
        .to_str = val__no_to_str,
        .to_pk = (ti_val_to_pk_cb) val__future_to_pk,
        .get_type_str = val__future_type_str,
        .allowed_as_vtask_arg = false,
    },
    /* TI_VAL_TEMPLATE */
    {
        .destroy = (ti_val_destroy_cb) ti_template_destroy,
        .allowed_as_vtask_arg = false,
    },
};

#define ti_val(__val) (&ti_val_type_props[(__val)->tp])

static inline const char * ti_val_str(ti_val_t * val)
{
    return ti_val(val)->get_type_str(val);
}

static inline int ti_val_to_pk(ti_val_t * val, ti_vp_t * vp, int options)
{
    return ti_val(val)->to_pk(val, vp, options);
}

/*
 * -------------------------  End value methods ----------------------------
 */

static inline void ti_val_drop(ti_val_t * val)
{
    if (val && !--val->ref)
        ti_val(val)->destroy(val);
}

static inline void ti_val_unsafe_drop(ti_val_t * val)
{
    if (!--val->ref)
        ti_val(val)->destroy(val);
}

static inline void ti_val_unsafe_gc_drop(ti_val_t * val)
{
    if (!--val->ref)
        ti_val(val)->destroy(val);
    else
        ti_thing_may_push_gc((ti_thing_t *) val);
}

static inline void ti_val_gc_drop(ti_val_t * val)
{
    if (val)
        ti_val_unsafe_gc_drop(val);
}

static inline void ti_val_unassign_unsafe_drop(ti_val_t * val)
{
    if (!--val->ref)
        ti_val(val)->destroy(val);
    else if (val->tp == TI_VAL_SET || val->tp == TI_VAL_ARR)
        ((ti_varr_t *) val)->parent = NULL;
    else
        ti_thing_may_push_gc((ti_thing_t *) val);
}

static inline void ti_val_unassign_drop(ti_val_t * val)
{
    if (val)
        ti_val_unassign_unsafe_drop(val);
}

static inline _Bool ti_val_is_arr(ti_val_t * val)
{
    return val->tp == TI_VAL_ARR;
}

static inline _Bool ti_val_is_bool(ti_val_t * val)
{
    return val->tp == TI_VAL_BOOL;
}

static inline _Bool ti_val_is_datetime(ti_val_t * val)
{
    return val->tp == TI_VAL_DATETIME;
}

static inline _Bool ti_val_is_datetime_strict(ti_val_t * val)
{
    return val->tp == TI_VAL_DATETIME &&
            ti_datetime_is_datetime((ti_datetime_t *) val);
}

static inline _Bool ti_val_is_timeval(ti_val_t * val)
{
    return val->tp == TI_VAL_DATETIME &&
            ti_datetime_is_timeval((ti_datetime_t *) val);
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

static inline _Bool ti_val_is_uint(ti_val_t * val)
{
    return val->tp == TI_VAL_INT && VINT(val) >= 0;
}

static inline _Bool ti_val_is_pint(ti_val_t * val)
{
    return val->tp == TI_VAL_INT && VINT(val) > 0;
}

static inline _Bool ti_val_is_nint(ti_val_t * val)
{
    return val->tp == TI_VAL_INT && VINT(val) < 0;
}

static inline _Bool ti_val_is_nil(ti_val_t * val)
{
    return val->tp == TI_VAL_NIL;
}

static inline _Bool ti_val_is_str(ti_val_t * val)
{
    return val->tp == TI_VAL_STR || val->tp == TI_VAL_NAME;
}

static inline _Bool ti_val_is_utf8(ti_val_t * val)
{
    return (val->tp == TI_VAL_STR || val->tp == TI_VAL_NAME) && strx_is_utf8n(
        ((ti_str_t *) val)->str,
        ((ti_str_t *) val)->n);
}

static inline _Bool ti_val_is_str_nil(ti_val_t * val)
{
    return val->tp == TI_VAL_STR ||
           val->tp == TI_VAL_NAME ||
           val->tp == TI_VAL_NIL;
}

static inline _Bool ti_val_is_str_regex(ti_val_t * val)
{
    return val->tp == TI_VAL_STR ||
           val->tp == TI_VAL_NAME ||
           val->tp == TI_VAL_REGEX;
}

static inline _Bool ti_val_is_str_closure(ti_val_t * val)
{
    return val->tp == TI_VAL_STR ||
           val->tp == TI_VAL_NAME ||
           val->tp == TI_VAL_CLOSURE;
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
           val->tp == TI_VAL_MPDATA;
}

static inline _Bool ti_val_is_str_bytes_nil(ti_val_t * val)
{
    return val->tp == TI_VAL_STR ||
           val->tp == TI_VAL_NAME ||
           val->tp == TI_VAL_BYTES ||
           val->tp == TI_VAL_NIL;
}

static inline _Bool ti_val_is_mpdata(ti_val_t * val)
{
    return val->tp == TI_VAL_MPDATA;
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

static inline _Bool ti_val_is_room(ti_val_t * val)
{
    return val->tp == TI_VAL_ROOM;
}

static inline _Bool ti_val_is_task(ti_val_t * val)
{
    return val->tp == TI_VAL_TASK;
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

static inline _Bool ti_val_is_future(ti_val_t * val)
{
    return val->tp == TI_VAL_FUTURE;
}

static inline _Bool ti_val_overflow_cast(double d)
{
    return !(d >= -VAL__CAST_MAX && d < VAL__CAST_MAX);
}

static inline _Bool ti_val_is_mut_locked(ti_val_t * val)
{
    return (val->tp == TI_VAL_ARR || val->tp == TI_VAL_SET) &&
           (val->flags & TI_VFLAG_LOCK);
}

/*
 * Names
 */
static inline ti_val_t * ti_val_year_name(void)
{
    ti_incref(val__year_name);
    return val__year_name;
}

static inline ti_val_t * ti_val_month_name(void)
{
    ti_incref(val__month_name);
    return val__month_name;
}

static inline ti_val_t * ti_val_day_name(void)
{
    ti_incref(val__day_name);
    return val__day_name;
}

static inline ti_val_t * ti_val_hour_name(void)
{
    ti_incref(val__hour_name);
    return val__hour_name;
}

static inline ti_val_t * ti_val_minute_name(void)
{
    ti_incref(val__minute_name);
    return val__minute_name;
}

static inline ti_val_t * ti_val_second_name(void)
{
    ti_incref(val__second_name);
    return val__second_name;
}

static inline ti_val_t * ti_val_gmt_offset_name(void)
{
    ti_incref(val__gmt_offset_name);
    return val__gmt_offset_name;
}

static inline ti_val_t * ti_val_borrow_year_name(void)
{
    return val__year_name;
}

static inline ti_val_t * ti_val_borrow_month_name(void)
{
    return val__month_name;
}

static inline ti_val_t * ti_val_borrow_day_name(void)
{
    return val__day_name;
}

static inline ti_val_t * ti_val_borrow_hour_name(void)
{
    return val__hour_name;
}

static inline ti_val_t * ti_val_borrow_minute_name(void)
{
    return val__minute_name;
}

static inline ti_val_t * ti_val_borrow_second_name(void)
{
    return val__second_name;
}

static inline ti_val_t * ti_val_borrow_module_name(void)
{
    return val__module_name;
}

static inline ti_val_t * ti_val_borrow_deep_name(void)
{
    return val__deep_name;
}

static inline ti_val_t * ti_val_borrow_load_name(void)
{
    return val__load_name;
}

static inline ti_val_t * ti_val_borrow_beautify_name(void)
{
    return val__beautify_name;
}

static inline ti_val_t * ti_val_any_str(void)
{
    ti_incref(val__sany);
    return val__sany;
}

static inline ti_val_t * ti_val_nil_str(void)
{
    ti_incref(val__snil);
    return val__snil;
}

static inline ti_val_t * ti_val_true_str(void)
{
    ti_incref(val__strue);
    return val__strue;
}

static inline ti_val_t * ti_val_false_str(void)
{
    ti_incref(val__sfalse);
    return val__sfalse;
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
        ex_set(e, EX_OPERATION,
            "cannot change type `%s` while the value is in use",
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
        void * key)  /* ti_raw_t or ti_name_t */
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_DATETIME:
    case TI_VAL_MPDATA:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ROOM:
    case TI_VAL_TASK:
    case TI_VAL_ERROR:
    case TI_VAL_MEMBER:
    case TI_VAL_CLOSURE:
    case TI_VAL_FUTURE:
        return;
    case TI_VAL_ARR:
        ((ti_varr_t *) val)->parent = parent;
        ((ti_varr_t *) val)->key_ = key;
        return;
    case TI_VAL_SET:
        ((ti_vset_t *) val)->parent = parent;
        ((ti_vset_t *) val)->key_ = key;
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
        void * key,
        ex_t * e)
{
    switch ((ti_val_enum) (*val)->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_DATETIME:
    case TI_VAL_MPDATA:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ROOM:
    case TI_VAL_TASK:
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
        ((ti_varr_t *) *val)->key_ = key;
        return 0;
    case TI_VAL_SET:
        if (ti_vset_assign((ti_vset_t **) val))
        {
            ex_set_mem(e);
            return e->nr;
        }
        ((ti_vset_t *) *val)->parent = parent;
        ((ti_vset_t *) *val)->key_ = key;
        return 0;
    case TI_VAL_CLOSURE:
        return ti_closure_unbound((ti_closure_t *) *val, e);
    case TI_VAL_FUTURE:
        ti_val_unsafe_drop(*val);
        *val = (ti_val_t *) ti_nil_get();
        return 0;
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
    case TI_VAL_DATETIME:
    case TI_VAL_MPDATA:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ROOM:
    case TI_VAL_TASK:
    case TI_VAL_ERROR:
    case TI_VAL_MEMBER:
    case TI_VAL_FUTURE:
        return 0;
    case TI_VAL_ARR:
        if (((ti_varr_t *) *val)->parent &&
            ti_varr_is_list((ti_varr_t *) *val) &&
            ti_varr_to_list((ti_varr_t **) val))
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

static inline int val__str_to_str(ti_val_t ** UNUSED(v), ex_t * UNUSED(e))
{
    return 0;
}
static inline int val__no_to_str(ti_val_t ** val, ex_t * e)
{
    ex_set(e, EX_TYPE_ERROR,
            "cannot convert type `%s` to `"TI_VAL_STR_S"`",
            ti_val_str(*val));
    return e->nr;
}
static inline int val__future_to_pk(ti_future_t * future, ti_vp_t * vp, int options)
{
    assert (options >= 0);
    return future->rval
            ? ti_val_to_pk(future->rval, vp, options)
            : msgpack_pack_nil(&vp->pk);
}

static inline int val__member_to_pk(ti_member_t * member, ti_vp_t * vp, int options)
{
    return options >= 0
        ? ti_val(member->val)->to_pk(member->val, vp, options)
        : -(msgpack_pack_map(&vp->pk, 1) ||
            mp_pack_strn(&vp->pk, TI_KIND_S_MEMBER, 1) ||
            msgpack_pack_array(&vp->pk, 2) ||
            msgpack_pack_uint16(&vp->pk, member->enum_->enum_id) ||
            msgpack_pack_uint16(&vp->pk, member->idx));
}

static inline int val__varr_to_pk(ti_varr_t * varr, ti_vp_t * vp, int options)
{
    if (msgpack_pack_array(&vp->pk, varr->vec->n))
        return -1;
    for (vec_each(varr->vec, ti_val_t, v))
        if (ti_val_to_pk(v, vp, options))
            return -1;
    return 0;
}


typedef _Bool (*ti_val_spec_cb) (ti_val_t *);

typedef struct
{
    ti_val_spec_cb is_spec;
} ti_val_spec_t;


static inline _Bool val__spec_enum_eq_to_val(uint16_t spec, ti_val_t * val)
{
    return (
        ti_val_is_member(val) &&
        ti_member_enum_id((ti_member_t *) val) == (spec & TI_ENUM_ID_MASK)
    );
}

static ti_val_spec_t ti_val_spec_map[20] = {
        {.is_spec=ti_val_is_thing},
        {.is_spec=ti_val_is_raw},
        {.is_spec=ti_val_is_str},
        {.is_spec=ti_val_is_utf8},
        {.is_spec=ti_val_is_bytes},
        {.is_spec=ti_val_is_int},
        {.is_spec=ti_val_is_uint},
        {.is_spec=ti_val_is_pint},
        {.is_spec=ti_val_is_nint},
        {.is_spec=ti_val_is_float},
        {.is_spec=ti_val_is_number},
        {.is_spec=ti_val_is_bool},
        {.is_spec=ti_val_is_array},
        {.is_spec=ti_val_is_set},
        {.is_spec=ti_val_is_datetime_strict},
        {.is_spec=ti_val_is_timeval},
        {.is_spec=ti_val_is_regex},
        {.is_spec=ti_val_is_closure},
        {.is_spec=ti_val_is_error},
        {.is_spec=ti_val_is_room},
};

static inline _Bool ti_val_is_spec(ti_val_t * val, uint16_t spec)
{
    if (spec == TI_SPEC_ANY ||
        ((spec & TI_SPEC_NILLABLE) && ti_val_is_nil(val)))
        return true;

    if (spec > TI_SPEC_ANY)
        return ti_val_spec_map[spec-TI_SPEC_OBJECT].is_spec(val);

    if (spec >= TI_ENUM_ID_FLAG)
        return val__spec_enum_eq_to_val(spec, val);

    return ti_val_is_thing(val) && ((ti_thing_t *) val)->type_id == spec;
}

#endif  /* TI_VAL_INLINE_H_ */



