/*
 * ti/val.h
 */
#ifndef TI_VAL_H_
#define TI_VAL_H_

#define VAL__CAST_MAX 9223372036854775808.0

typedef enum
{
    TI_VAL_NIL,
    TI_VAL_INT,
    TI_VAL_FLOAT,
    TI_VAL_BOOL,
    TI_VAL_QP,      /* QPack data, only used on root for returning raw packed
                       data */
    TI_VAL_RAW,
    TI_VAL_REGEX,
    TI_VAL_THING,
    TI_VAL_ARR,     /* array, list or tuple */
    TI_VAL_SET,     /* set of things */
    TI_VAL_CLOSURE,
    TI_VAL_ERROR,
} ti_val_enum;

#define TI_VAL_NIL_S        "nil"
#define TI_VAL_INT_S        "int"
#define TI_VAL_FLOAT_S      "float"
#define TI_VAL_BOOL_S       "bool"
#define TI_VAL_QP_S         "qpack"
#define TI_VAL_RAW_S        "raw"
#define TI_VAL_REGEX_S      "regex"
#define TI_VAL_THING_S      "thing"
#define TI_VAL_ARR_S        "array"
#define TI_VAL_ARR_LIST_S   "list"
#define TI_VAL_ARR_TUPLE_S  "tuple"
#define TI_VAL_SET_S        "set"
#define TI_VAL_CLOSURE_S    "closure"
#define TI_VAL_ERROR_S      "error"

enum
{
    TI_VFLAG_THING_SWEEP     =1<<0,      /* marked for sweep; each thing is
                                            initially marked and while running
                                            garbage collection this mark is
                                            removed as long as the `thing` is
                                            attached to the collection. */
    TI_VFLAG_THING_NEW       =1<<1,      /* thing is new; new things require
                                            a full `dump` in a task while
                                            existing things only can contain
                                            the `id`.*/
    TI_VFLAG_CLOSURE_BTSCOPE =1<<2,      /* closure bound to query string
                                            within the ThingsDB scope;
                                            when not stored, closures do not
                                            own the closure string but refer
                                            the full query string.*/
    TI_VFLAG_CLOSURE_BCSCOPE =1<<3,      /* closure bound to query string
                                            within a collection scope;
                                            when not stored, closures do not
                                            own the closure string but refer
                                            the full query string. */
    TI_VFLAG_CLOSURE_WSE     =1<<4,      /* stored closure with side effects;
                                            when closure make changes they
                                            require an event and thus must be
                                            wrapped by wse() so we can know
                                            an event is created.
                                            (only stored closures) */
    TI_VFLAG_LOCK            =1<<5,      /* closure or thing in use;
                                            required for recursion or iteration
                                            detection. */
    TI_VFLAG_ARR_TUPLE       =1<<6,      /* array is immutable; nested, and
                                            only nested array's are tuples;
                                            once a tuple is direct assigned to
                                            a thing, it converts back to a
                                            mutable list. */
    TI_VFLAG_ARR_MHT         =1<<7,      /* array may-have-things; some code
                                            might skip arrays without this flag
                                            while searching for things; */
};

typedef enum
{
    /*
     * Reserved:
     *   + positive big type
     *   - negative big type
     *   @ date type
     */
    TI_KIND_C_THING     ='#',
    TI_KIND_C_CLOSURE   ='>',
    TI_KIND_C_REGEX     ='*',
    TI_KIND_C_SET       ='$',
    TI_KIND_C_ERROR     ='!',

} ti_val_kind;

#define TI_KIND_S_THING     "#"
#define TI_KIND_S_CLOSURE   ">"
#define TI_KIND_S_REGEX     "*"
#define TI_KIND_S_SET       "$"
#define TI_KIND_S_ERROR     "!"

typedef struct ti_val_s ti_val_t;

#include <qpack.h>
#include <stdint.h>
#include <ti/ex.h>
#include <util/imap.h>
#include <util/vec.h>
#include <ti/varr.h>

int ti_val_init_common(void);
void ti_val_drop_common(void);
void ti_val_destroy(ti_val_t * val);
int ti_val_make_int(ti_val_t ** val, int64_t i);
int ti_val_make_float(ti_val_t ** val, double d);
ti_val_t * ti_val_from_unp(qp_unpacker_t * unp, imap_t * things);
ti_val_t * ti_val_empty_str(void);
vec_t ** ti_val_get_access(ti_val_t * val, ex_t * e, uint64_t * target_id);
int ti_val_convert_to_str(ti_val_t ** val);
int ti_val_convert_to_int(ti_val_t ** val, ex_t * e);
int ti_val_convert_to_float(ti_val_t ** val, ex_t * e);
int ti_val_convert_to_array(ti_val_t ** val, ex_t * e);
int ti_val_convert_to_set(ti_val_t ** val, ex_t * e);
_Bool ti_val_as_bool(ti_val_t * val);
_Bool ti_val_is_valid_name(ti_val_t * val);
size_t ti_val_get_len(ti_val_t * val);
int ti_val_gen_ids(ti_val_t * val);
int ti_val_to_packer(ti_val_t * val, qp_packer_t ** packer, int options);
int ti_val_to_file(ti_val_t * val, FILE * f);
const char * ti_val_str(ti_val_t * val);
int ti_val_make_assignable(ti_val_t ** val, ex_t * e);
static inline _Bool ti_val_is_arr(ti_val_t * val);
static inline _Bool ti_val_is_array(ti_val_t * val);
static inline _Bool ti_val_is_bool(ti_val_t * val);
static inline _Bool ti_val_is_closure(ti_val_t * val);
static inline _Bool ti_val_is_error(ti_val_t * val);
static inline _Bool ti_val_is_float(ti_val_t * val);
static inline _Bool ti_val_is_int(ti_val_t * val);
static inline _Bool ti_val_is_list(ti_val_t * val);
static inline _Bool ti_val_is_nil(ti_val_t * val);
static inline _Bool ti_val_is_raw(ti_val_t * val);
static inline _Bool ti_val_is_regex(ti_val_t * val);
static inline _Bool ti_val_is_set(ti_val_t * val);
static inline _Bool ti_val_is_thing(ti_val_t * val);
static inline _Bool ti_val_has_len(ti_val_t * val);
static inline _Bool ti_val_overflow_cast(double d);
static inline void ti_val_drop(ti_val_t * val);
static inline int ti_val_ensure_lock(ti_val_t * val);
static inline void ti_val_unlock(ti_val_t * val);
struct ti_val_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    uint16_t _pad16;
};

static inline void ti_val_drop(ti_val_t * val)
{
    if (val && !--val->ref)
        ti_val_destroy(val);
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

static inline _Bool ti_val_is_raw(ti_val_t * val)
{
    return val->tp == TI_VAL_RAW;
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
        val->tp == TI_VAL_RAW ||
        val->tp == TI_VAL_THING ||
        val->tp == TI_VAL_ARR ||
        val->tp == TI_VAL_SET ||
        val->tp == TI_VAL_ERROR
    );
}

static inline _Bool ti_val_is_locked(ti_val_t * val, ex_t * e)
{
    if (val->flags & TI_VFLAG_LOCK)
    {
        ex_set(e, EX_BAD_DATA,
            "cannot change type `%s` while the value is in use",
            ti_val_str(val));
        return true;
    }
    return false;
}

static inline _Bool ti_val_overflow_cast(double d)
{
    return !(d >= -VAL__CAST_MAX && d < VAL__CAST_MAX);
}

/* returns `lock_was_set`: 0 if already locked, 1 if a new lock is set */
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


#endif /* TI_VAL_H_ */
