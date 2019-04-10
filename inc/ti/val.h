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
    TI_VAL_CLOSURE,
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
#define TI_VAL_CLOSURE_S    "closure"

typedef enum
{
    TI_VAL_KIND_THING   ='#',
    TI_VAL_KIND_CLOSURE ='$',
    TI_VAL_KIND_REGEX   ='*',
} ti_val_kind;

enum
{
    TI_VAL_PACK_NEW     =1<<0,
};

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
int ti_val_convert_to_str(ti_val_t ** val);
int ti_val_convert_to_int(ti_val_t ** val, ex_t * e);
int ti_val_convert_to_errnr(ti_val_t ** val, ex_t * e);
_Bool ti_val_as_bool(ti_val_t * val);
_Bool ti_val_is_valid_name(ti_val_t * val);
size_t ti_val_iterator_n(ti_val_t * val);
int ti_val_gen_ids(ti_val_t * val);
int ti_val_to_packer(ti_val_t * val, qp_packer_t ** packer, int flags, int fetch);
int ti_val_to_file(ti_val_t * val, FILE * f);
const char * ti_val_str(ti_val_t * val);
int ti_val_make_assignable(ti_val_t * val, ex_t * e);
static inline _Bool ti_val_is_arr(ti_val_t * val);
static inline _Bool ti_val_is_closure(ti_val_t * val);
static inline _Bool ti_val_is_bool(ti_val_t * val);
static inline _Bool ti_val_is_float(ti_val_t * val);
static inline _Bool ti_val_is_int(ti_val_t * val);
static inline _Bool ti_val_is_nil(ti_val_t * val);
static inline _Bool ti_val_is_raw(ti_val_t * val);
static inline _Bool ti_val_is_regex(ti_val_t * val);
static inline _Bool ti_val_is_indexable(ti_val_t * val);
static inline _Bool ti_val_is_iterable(ti_val_t * val);
static inline _Bool ti_val_is_array(ti_val_t * val);
static inline _Bool ti_val_is_list(ti_val_t * val);
static inline _Bool ti_val_is_settable(ti_val_t * val);
static inline _Bool ti_val_overflow_cast(double d);
static inline void ti_val_drop(ti_val_t * val);

struct ti_val_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _pad8;
    uint16_t _pad16;
};

static inline void ti_val_drop(ti_val_t * val)
{
    if (val && !--val->ref) ti_val_destroy(val);
}

static inline _Bool ti_val_is_arr(ti_val_t * val)
{
    return val->tp == TI_VAL_ARR;
}

static inline _Bool ti_val_is_closure(ti_val_t * val)
{
    return val->tp == TI_VAL_CLOSURE;
}

static inline _Bool ti_val_is_bool(ti_val_t * val)
{
    return val->tp == TI_VAL_BOOL;
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


static inline _Bool ti_val_is_indexable(ti_val_t * val)
{
    return val->tp == TI_VAL_RAW || val->tp == TI_VAL_ARR;
}

static inline _Bool ti_val_is_iterable(ti_val_t * val)
{
    return (
        val->tp == TI_VAL_RAW ||
        val->tp == TI_VAL_ARR ||
        val->tp == TI_VAL_THING
    );
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

static inline _Bool ti_val_overflow_cast(double d)
{
    return !(d >= -VAL__CAST_MAX && d < VAL__CAST_MAX);
}

static inline _Bool ti_val_is_settable(ti_val_t * val)
{
    return (
            val->tp == TI_VAL_INT ||
            val->tp == TI_VAL_FLOAT ||
            val->tp == TI_VAL_BOOL ||
            val->tp == TI_VAL_RAW
    );
}

#endif /* TI_VAL_H_ */
