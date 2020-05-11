/*
 * ti/enum.h
 */
#ifndef TI_ENUM_H_
#define TI_ENUM_H_


#define TI_ENUM_ID_FLAG 0x6000  /* SPEC values use this mask */
#define TI_ENUM_ID_MASK 0x1fff  /* subtract enum value from SPEC */
#define TI_ENUM_ID_NONE 0x2000  /* when an enum type is removed */

typedef struct ti_enum_s ti_enum_t;

#include <stdlib.h>
#include <inttypes.h>
#include <ti/enums.h>
#include <util/vec.h>


ti_enum_t * ti_enum_create(
        ti_enums_t * enums,
        uint16_t enum_id,
        const char * name,
        size_t name_n,
        uint64_t created_at,
        uint64_t modified_at);
void ti_enum_drop(ti_enum_t * enum_);
void ti_enum_destroy(ti_enum_t * enum_);

struct ti_enum_s
{
    uint32_t refcount;      /* this counter will be incremented and decremented
                               by other types; the enum may only be removed
                               when this counter is equal to zero otherwise
                               other types will break; */
    uint16_t enum_id;       /* enum id */
    uint16_t spec;          /* only these type specifications are allowed:
                               TI_SPEC_OBJECT, TI_SPEC_STR, TI_SPEC_BYTES,
                               TI_SPEC_FLOAT, TI_SPEC_BOOL */
    char * name;            /* name (null terminated) */
    char * wname;           /* wrapped name (null terminated) */
    ti_raw_t * rname;       /* name as raw type */
    vec_t * venums;
};

static inline ti_venum_t * ti_enum_by_idx(ti_enum_t * enum_, uint16_t idx)
{
    return vec_get_or_null(enum_->venums, idx);
}

#endif  /* TI_VENUM_H_ */
