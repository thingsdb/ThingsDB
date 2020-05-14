/*
 * ti/member.h
 */
#ifndef TI_MEMBER_H_
#define TI_MEMBER_H_

#define VMEMBER(__x) ((ti_member_t *) (__x))->val

#include <stdlib.h>
#include <inttypes.h>
#include <ti/val.h>
#include <ti/enum.h>

typedef struct ti_member_s ti_member_t;

ti_member_t * ti_member_create(
        ti_enum_t * enum_,
        ti_name_t * name,
        ti_val_t * val,
        uint16_t idx);
void ti_member_destroy(ti_member_t * member);

#define VINT(__x) ((ti_vint_t *) (__x))->int_

struct ti_member_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _flags;
    uint16_t member_idx;    /* index in enum_->vec */
    ti_enum_t * enum_;
    ti_name_t * name;       /* with reference */
    ti_val_t * val;         /* with reference */
};

#endif  /* TI_MEMBER_H_ */
