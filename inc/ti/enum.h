/*
 * ti/enum.h
 */
#ifndef TI_ENUM_H_
#define TI_ENUM_H_


#define TI_ENUM_ID_FLAG 0x6000  /* SPEC values use this mask */
#define TI_ENUM_ID_MASK 0x1fff  /* subtract enum value from SPEC */

typedef struct ti_enum_s ti_enum_t;

enum
{
    TI_ENUM_FLAG_LOCK       =1<<0,
};

#include <ex.h>
#include <stdlib.h>
#include <inttypes.h>
#include <ti/enums.h>
#include <ti/thing.h>
#include <ti/venum.h>
#include <util/vec.h>

typedef int (*enum_conv_cb)(ti_val_t **, ex_t *);

ti_enum_t * ti_enum_create(
        ti_enums_t * enums,
        uint16_t enum_id,
        const char * name,
        size_t name_n,
        uint64_t created_at,
        uint64_t modified_at);
void ti_enum_drop(ti_enum_t * enum_);
void ti_enum_destroy(ti_enum_t * enum_);
int ti_enum_init_from_thing(ti_enum_t * enum_, ti_thing_t * thing, ex_t * e);
ti_venum_t * ti_enum_val_by_val_e(ti_enum_t * enum_, ti_val_t * val, ex_t * e);

typedef enum
{
    TI_ENUM_INT,
    TI_ENUM_FLOAT,
    TI_ENUM_STR,
    TI_ENUM_BYTES,
    TI_ENUM_THING,
} ti_enum_enum;

 uint16_t ti_enum_spec_map[] = {
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

struct ti_enum_s
{
    uint32_t refcount;      /* this counter will be incremented and decremented
                               by other types; the enum may only be removed
                               when this counter is equal to zero otherwise
                               other types will break; */
    uint16_t enum_id;       /* enum id */
    uint8_t flags;          /* enum flags */
    uint8_t enum_tp;        /* enum tp */
    uint64_t created_at;    /* UNIX time-stamp in seconds */
    uint64_t modified_at;   /* UNIX time-stamp in seconds */
    enum_conv_cb conv_cb;   /* conversion callback */
    char * name;            /* name (null terminated) */
    ti_raw_t * rname;       /* name as raw type */
    vec_t * vec;
    smap_t * smap;
};

static inline ti_venum_t * ti_enum_val_by_idx(ti_enum_t * enum_, uint16_t idx)
{
    return vec_get_or_null(enum_->vec, idx);
}

static inline uint16_t ti_enum_spec(ti_enum_t * enum_)
{
    return ti_enum_spec_map[enum_->enum_tp];
}

static inline const char * ti_enum_tp_str(ti_enum_t * enum_)
{
    return ti_enum_str_map[enum_->enum_tp];
}

static inline ti_venum_t * ti_enum_val_by_strn(
        ti_enum_t * enum_,
        const char * str,
        size_t n)
{
    return smap_getn(enum_->smap, str, n);
}

static inline ti_venum_t * ti_enum_val_by_raw(
        ti_enum_t * enum_,
        ti_raw_t * raw)
{
    return smap_getn(enum_->smap, (const char *) raw->data, raw->n);
}

static inline ti_venum_t * ti_enum_val_by_val(
        ti_enum_t * enum_,
        ti_val_t * val)
{
    for(vec_each(enum_->vec, ti_venum_t, venum))
        if (ti_opr_eq(VENUM(venum), val))
            return venum;
    return NULL;
}



static inline int ti_enum_try_lock(ti_enum_t * enum_, ex_t * e)
{
    if (enum_->flags & TI_ENUM_FLAG_LOCK)
    {
        ex_set(e, EX_OPERATION_ERROR,
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

#endif  /* TI_VENUM_H_ */
