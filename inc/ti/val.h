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
    TI_VAL_MP,          /* msgpack data */
    TI_VAL_NAME,
    TI_VAL_STR,
    TI_VAL_BYTES,       /* MP,STR and BIN all use RAW as underlying type */
    TI_VAL_REGEX,
    TI_VAL_THING,       /* instance or object */
    TI_VAL_WRAP,
    TI_VAL_ARR,         /* array, list or tuple */
    TI_VAL_SET,         /* set of things */
    TI_VAL_CLOSURE,
    TI_VAL_ERROR,
    TI_VAL_ENUM,
    TI_VAL_TEMPLATE,    /* template to generate TI_VAL_STR
                           note that a template is never stored like a value,
                           rather it may build from either a query or a stored
                           closure; therefore template does not need to be
                           handled like all other value type. */
} ti_val_enum;

#define TI_VAL_NIL_S        "nil"
#define TI_VAL_INT_S        "int"
#define TI_VAL_FLOAT_S      "float"
#define TI_VAL_BOOL_S       "bool"
#define TI_VAL_INFO_S       "info"
#define TI_VAL_STR_S        "str"
#define TI_VAL_BYTES_S      "bytes"
#define TI_VAL_REGEX_S      "regex"
#define TI_VAL_THING_S      "thing"
#define TI_VAL_LIST_S       "list"
#define TI_VAL_TUPLE_S      "tuple"
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
                                            within the thingsdb scope;
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
    TI_VFLAG_LOCK            =1<<5,      /* thing or value in use;
                                            used to prevent illegal changes */
    TI_VFLAG_ARR_TUPLE       =1<<6,      /* array is immutable; nested, and
                                            only nested array's are tuples;
                                            once a tuple is direct assigned to
                                            a thing, it converts back to a
                                            mutable list. */
    TI_VFLAG_ARR_MHT         =1<<7,      /* array may-have-things; some code
                                            might skip arrays without this flag
                                            while searching for things; */
};

/* negative value is used for packing tasks */
#define TI_VAL_PACK_TASK -1
#define TI_VAL_PACK_FILE -2

typedef enum
{
    /*
     * Reserved (may be implemented in the future):
     *   All specials are in the binary 0010xxxx range.
     *   + positive big type
     *   - negative big type
     *   % date type
     */
    TI_KIND_C_THING     ='#',
    TI_KIND_C_INSTANCE  ='.',
    TI_KIND_C_CLOSURE   ='/',
    TI_KIND_C_REGEX     ='*',
    TI_KIND_C_SET       ='$',
    TI_KIND_C_ERROR     ='!',
    TI_KIND_C_WRAP      ='&',
    TI_KIND_C_ENUM      =',',
} ti_val_kind;

#define TI_KIND_S_THING     "#"
#define TI_KIND_S_INSTANCE  "."
#define TI_KIND_S_CLOSURE   "/"
#define TI_KIND_S_REGEX     "*"
#define TI_KIND_S_SET       "$"
#define TI_KIND_S_ERROR     "!"
#define TI_KIND_S_WRAP      "&"
#define TI_KIND_S_ENUM      ","

typedef struct ti_val_s ti_val_t;

#include <stdint.h>
#include <tiinc.h>
#include <ex.h>
#include <ti/vup.h>
#include <util/imap.h>
#include <util/vec.h>
#include <ti/varr.h>
#include <ti/collection.h>

int ti_val_init_common(void);
void ti_val_drop_common(void);
void ti_val_destroy(ti_val_t * val);
int ti_val_make_int(ti_val_t ** val, int64_t i);
int ti_val_make_float(ti_val_t ** val, double d);
ti_val_t * ti_val_from_unp(ti_vup_t * vup);
ti_val_t * ti_val_from_unp_e(ti_vup_t * vup, ex_t * e);
ti_val_t * ti_val_empty_str(void);
ti_val_t * ti_val_borrow_tar_gz_str(void);
ti_val_t * ti_val_empty_bin(void);
ti_val_t * ti_val_wthing_str(void);
vec_t ** ti_val_get_access(ti_val_t * val, ex_t * e, uint64_t * scope_id);
int ti_val_convert_to_str(ti_val_t ** val, ex_t * e);
int ti_val_convert_to_bytes(ti_val_t ** val, ex_t * e);
int ti_val_convert_to_int(ti_val_t ** val, ex_t * e);
int ti_val_convert_to_float(ti_val_t ** val, ex_t * e);
int ti_val_convert_to_array(ti_val_t ** val, ex_t * e);
int ti_val_convert_to_set(ti_val_t ** val, ex_t * e);
_Bool ti_val_as_bool(ti_val_t * val);
_Bool ti_val_is_valid_name(ti_val_t * val);
size_t ti_val_get_len(ti_val_t * val);
int ti_val_gen_ids(ti_val_t * val);
int ti_val_to_pk(ti_val_t * val, msgpack_packer * pk, int options);
void ti_val_may_change_pack_sz(ti_val_t * val, size_t * sz);
const char * ti_val_str(ti_val_t * val);
ti_val_t * ti_val_strv(ti_val_t * val);
static inline _Bool ti_val_is_arr(ti_val_t * val);
static inline _Bool ti_val_is_array(ti_val_t * val);
static inline _Bool ti_val_is_bool(ti_val_t * val);
static inline _Bool ti_val_is_closure(ti_val_t * val);
static inline _Bool ti_val_is_error(ti_val_t * val);
static inline _Bool ti_val_is_number(ti_val_t * val);
static inline _Bool ti_val_is_float(ti_val_t * val);
static inline _Bool ti_val_is_int(ti_val_t * val);
static inline _Bool ti_val_is_list(ti_val_t * val);
static inline _Bool ti_val_is_nil(ti_val_t * val);
static inline _Bool ti_val_is_str(ti_val_t * val);
static inline _Bool ti_val_is_bytes(ti_val_t * val);
static inline _Bool ti_val_is_raw(ti_val_t * val);
static inline _Bool ti_val_is_regex(ti_val_t * val);
static inline _Bool ti_val_is_set(ti_val_t * val);
static inline _Bool ti_val_is_thing(ti_val_t * val);
static inline _Bool ti_val_is_wrap(ti_val_t * val);
static inline _Bool ti_val_has_len(ti_val_t * val);
static inline _Bool ti_val_overflow_cast(double d);
static inline void ti_val_drop(ti_val_t * val);
static inline int ti_val_try_lock(ti_val_t * val, ex_t * e);
static inline int ti_val_ensure_lock(ti_val_t * val);
static inline void ti_val_unlock(ti_val_t * val, int lock_was_set);

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

#endif /* TI_VAL_H_ */
