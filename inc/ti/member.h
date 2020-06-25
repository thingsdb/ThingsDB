/*
 * ti/member.h
 */
#ifndef TI_MEMBER_H_
#define TI_MEMBER_H_


#include <ex.h>
#include <stdint.h>
#include <ti/enum.t.h>
#include <ti/member.t.h>
#include <ti/name.t.h>
#include <ti/val.t.h>

ti_member_t * ti_member_create(
        ti_enum_t * enum_,
        ti_name_t * name,
        ti_val_t * val,
        ex_t * e);
void ti_member_destroy(ti_member_t * member);
void ti_member_drop(ti_member_t * member);
void ti_member_remove(ti_member_t * member);
void ti_member_del(ti_member_t * member);
void ti_member_def(ti_member_t * member);
int ti_member_set_value(ti_member_t * member, ti_val_t * val, ex_t * e);
int ti_member_set_name(
        ti_member_t * member,
        const char * s,
        size_t n,
        ex_t * e);

#endif  /* TI_MEMBER_H_ */
