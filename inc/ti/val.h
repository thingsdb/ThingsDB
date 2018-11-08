/*
 * val.h
 */
#ifndef TI_VAL_H_
#define TI_VAL_H_

typedef enum
{
    TI_VAL_VAL,             /* stores a value from `set_props` */
    TI_VAL_UNDEFINED,
    TI_VAL_NIL,
    TI_VAL_INT,
    TI_VAL_FLOAT,
    TI_VAL_BOOL,
    TI_VAL_RAW,
    TI_VAL_ARRAY,   /* NEVER turn back to TI_VAL_THINGS */
    TI_VAL_THING,
    TI_VAL_THINGS,  /* when empty, this can be turned into TI_VAL_ARRAY */
} ti_val_enum;

typedef enum
{
    TI_VAL_FLAG_FETCH   =1<<0,
} ti_val_flags;

enum
{
    TI_VAL_PACK_FETCH,
    TI_VAL_PACK_NEW,
};

typedef struct ti_val_s ti_val_t;
typedef union ti_val_u ti_val_via_t;

#include <qpack.h>
#include <stdint.h>
#include <ti/raw.h>
#include <ti/thing.h>
#include <util/vec.h>

ti_val_t * ti_val_create(ti_val_enum tp, void * v);
ti_val_t * ti_val_weak_create(ti_val_enum tp, void * v);
ti_val_t * ti_val_weak_dup(ti_val_t * val);
void ti_val_destroy(ti_val_t * val);
void ti_val_weak_destroy(ti_val_t * val);
void ti_val_weak_set(ti_val_t * val, ti_val_enum tp, void * v);
int ti_val_set(ti_val_t * val, ti_val_enum tp, void * v);
void ti_val_weak_copy(ti_val_t * to, ti_val_t * from);
int ti_val_copy(ti_val_t * to, ti_val_t * from);
void ti_val_set_bool(ti_val_t * val, _Bool bool_);
void ti_val_set_nil(ti_val_t * val);
void ti_val_set_undefined(ti_val_t * val);
void ti_val_set_int(ti_val_t * val, int64_t i);
void ti_val_set_float(ti_val_t * val, double d);
int ti_val_gen_ids(ti_val_t * val);
void ti_val_clear(ti_val_t * val);
int ti_val_to_packer(ti_val_t * val, qp_packer_t ** packer, int pack);
int ti_val_to_file(ti_val_t * val, FILE * f);
const char * ti_val_to_str(ti_val_t * val);
static inline _Bool ti_val_is_array(ti_val_t * val);
static inline _Bool ti_val_is_indexable(ti_val_t * val);
static inline void ti_val_mark_fetch(ti_val_t * val);
static inline void ti_val_unmark_fetch(ti_val_t * val);

union ti_val_u
{
    ti_val_t * val;
    void * undefined;
    void * nil;
    int64_t int_;
    double float_;
    _Bool bool_;
    ti_raw_t * raw;
    vec_t * array;          /* ti_val_t*        */
    ti_thing_t * thing;
    vec_t * things;         /* ti_thing_t*      */
    vec_t * arr;            /* placeholder for array and things */
};

struct ti_val_s
{
    uint8_t tp;
    uint8_t flags;
    ti_val_via_t via;
};

static inline _Bool ti_val_is_array(ti_val_t * val)
{
    return (
        val->tp == TI_VAL_ARRAY ||
        val->tp == TI_VAL_THINGS
    );
}

static inline _Bool ti_val_is_indexable(ti_val_t * val)
{
    return (
        val->tp == TI_VAL_RAW ||
        val->tp == TI_VAL_ARRAY ||
        val->tp == TI_VAL_THINGS
    );
}

static inline void ti_val_mark_fetch(ti_val_t * val)
{
    val->flags |= TI_VAL_FLAG_FETCH;
}

static inline void ti_val_unmark_fetch(ti_val_t * val)
{
    val->flags &= ~TI_VAL_FLAG_FETCH;
}

#endif /* TI_VAL_H_ */
