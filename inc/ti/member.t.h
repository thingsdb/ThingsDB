/*
 * ti/member.t.h
 */
#ifndef TI_MEMBER_T_H_
#define TI_MEMBER_T_H_

#define VMEMBER(__x) ((ti_member_t *) (__x))->val

typedef struct ti_member_s ti_member_t;

#include <stdint.h>
#include <ti/enum.t.h>
#include <ti/name.t.h>
#include <ti/val.t.h>

struct ti_member_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _flags;
    uint16_t idx;           /* index in enum_->vec */
    ti_enum_t * enum_;      /* parent enum */
    ti_name_t * name;       /* with reference */
    ti_val_t * val;         /* with reference */
};

#endif  /* TI_MEMBER_T_H_ */
