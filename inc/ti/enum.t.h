/*
 * ti/enum.t.h
 */
#ifndef TI_ENUM_T_H_
#define TI_ENUM_T_H_

#define TI_ENUM_ID_FLAG 0x6000  /* SPEC values use this mask */
#define TI_ENUM_ID_MASK 0x1fff  /* subtract enum value from SPEC */

typedef struct ti_enum_s ti_enum_t;

enum
{
    TI_ENUM_FLAG_LOCK       =1<<0,
};

#include <ex.h>
#include <inttypes.h>
#include <ti/raw.t.h>
#include <ti/spec.t.h>
#include <ti/val.t.h>
#include <util/smap.h>
#include <util/vec.h>

typedef int (*enum_conv_cb)(ti_val_t **, ex_t *);

typedef enum
{
    TI_ENUM_INT,
    TI_ENUM_FLOAT,
    TI_ENUM_STR,
    TI_ENUM_BYTES,
    TI_ENUM_THING,
} ti_enum_enum;

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
    vec_t * members;        /* members stored by index */
    vec_t * methods;        /* ti_method_t */
    smap_t * smap;          /* member lookup by name */
};

#endif  /* TI_MEMBER_T_H_ */
