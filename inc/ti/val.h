/*
 * ti/val.h
 */
#ifndef TI_VAL_H_
#define TI_VAL_H_

#define VAL__CAST_MAX 9223372036854775808.0


typedef enum
{
    TI_VAL_ATTR,    /* attribute ti_prop_t */
    TI_VAL_NIL,
    TI_VAL_INT,
    TI_VAL_FLOAT,
    TI_VAL_BOOL,
    TI_VAL_QP,      /* QPack data, only used on root for returning raw packed
                       data */
    TI_VAL_RAW,
    TI_VAL_REGEX,
    TI_VAL_THING,
    TI_VAL_ARR,     /* array without things */
    TI_VAL_ARROW,
} ti_val_enum;

#define TI_VAL_ATTR_S       "attribute"
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
#define TI_VAL_ARROW_S      "arrow-function"

typedef enum
{
    TI_VAL_KIND_THING   ='#',
    TI_VAL_KIND_ARROW   ='$',
    TI_VAL_KIND_REGEX   ='*',
} ti_val_kind;

enum
{
    TI_VAL_PACK_NEW     =1<<0,
    TI_VAL_PACK_THING   =1<<1,
    TI_VAL_PACK_ATTR    =1<<2,
};

typedef struct ti_val_s ti_val_t;
typedef union ti_val_u ti_val_via_t;

#include <qpack.h>
#include <cleri/cleri.h>
#include <stdint.h>
#include <ti/raw.h>
#include <ti/thing.h>
#include <ti/name.h>
#include <ti/regex.h>
#include <ti/ex.h>
#include <ti/varr.h>
#include <util/vec.h>
#include <util/imap.h>

int ti_val_init_common(void);
void ti_val_drop_common(void);
void ti_val_drop(ti_val_t * val);
int ti_val_make_int(ti_val_t ** val, int64_t i);
int ti_val_make_float(ti_val_t ** val, double d);
int ti_val_make_raw(ti_val_t ** val, ti_raw_t * raw);
int ti_val_convert_to_str(ti_val_t ** val);
int ti_val_convert_to_int(ti_val_t ** val, ex_t * e);
int ti_val_convert_to_errnr(ti_val_t ** val, ex_t * e);
ti_val_t * ti_val_from_unp(qp_unpacker_t * unp, imap_t * things);
void __ti_val_weak_set(ti_val_t * val, ti_val_enum tp, void * v);
int ti_val_set(ti_val_t * val, ti_val_enum tp, void * v);
void ti_val_weak_copy(ti_val_t * to, ti_val_t * from);
int ti_val_copy(ti_val_t * to, ti_val_t * from);
_Bool ti_val_as_bool(ti_val_t * val);
_Bool ti_val_is_valid_name(ti_val_t * val);
size_t ti_val_iterator_n(ti_val_t * val);
int ti_val_gen_ids(ti_val_t * val);
void ti_val_clear(ti_val_t * val);
int ti_val_to_packer(ti_val_t * val, qp_packer_t ** packer, int flags);
int ti_val_to_file(ti_val_t * val, FILE * f);
const char * ti_val_str(ti_val_enum tp);
int ti_val_check_assignable(ti_val_t * val, _Bool to_array, ex_t * e);
static inline _Bool ti_val_is_arr(ti_val_t * val);
static inline _Bool ti_val_is_raw(ti_val_t * val);
static inline _Bool ti_val_is_indexable(ti_val_t * val);
static inline _Bool ti_val_is_iterable(ti_val_t * val);
static inline _Bool ti_val_is_array(ti_val_t * val);
static inline _Bool ti_val_is_list(ti_val_t * val);
static inline _Bool ti_val_overflow_cast(double d);

union ti_val_u
{
    void * attr;
    cleri_node_t * arrow;
    void * nil;
    int64_t int_;
    double float_;
    _Bool bool_;
    ti_raw_t * qp;
    ti_raw_t * raw;
    ti_regex_t * regex;
    vec_t * array;          /* ti_val_t*        */
    vec_t * tuple;          /* ti_val_t*        */
    ti_thing_t * thing;
    vec_t * things;         /* ti_thing_t*      */
    vec_t * arr;            /* placeholder for array and things */
};

struct ti_val_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _pad8;
    uint16_t _pad16;
};

static inline void ti_val_weak_destroy(ti_val_t * val)
{
    free(val);
}

static inline _Bool ti_val_is_arr(ti_val_t * val)
{
    return val->tp == TI_VAL_ARR;
}

static inline _Bool ti_val_is_raw(ti_val_t * val)
{
    return val->tp == TI_VAL_RAW;
}

static inline _Bool __ti_val_is_settable(ti_val_t * val)
{
    return (
        val->tp == TI_VAL_INT ||
        val->tp == TI_VAL_FLOAT ||
        val->tp == TI_VAL_BOOL ||
        val->tp == TI_VAL_RAW ||
        val->tp == TI_VAL_ARR);
}

static inline _Bool ti_val_is_indexable(ti_val_t * val)
{
    return (
        val->tp == TI_VAL_RAW ||
        val->tp == TI_VAL_ARR
    );
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


#endif /* TI_VAL_H_ */
