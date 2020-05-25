/*
 * ti/member.h
 */
#ifndef TI_MEMBER_H_
#define TI_MEMBER_H_

#define VMEMBER(__x) ((ti_member_t *) (__x))->val

typedef struct ti_member_s ti_member_t;

#include <stdlib.h>
#include <inttypes.h>
#include <ex.h>
#include <ti/val.h>
#include <ti/name.h>
#include <ti/enum.h>

ti_member_t * ti_member_create(
        ti_enum_t * enum_,
        ti_name_t * name,
        ti_val_t * val,
        ex_t * e);
void ti_member_destroy(ti_member_t * member);
void ti_member_drop(ti_member_t * member);
void ti_member_del(ti_member_t * member);
int ti_member_set_value(ti_member_t * member, ti_val_t * val, ex_t * e);
int ti_member_set_name(
        ti_member_t * member,
        const char * s,
        size_t n,
        ex_t * e);

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

#endif  /* TI_MEMBER_H_ */
