/*
 * ti/condition.h
 */
#ifndef TI_CONDITION_H_
#define TI_CONDITION_H_

#include <ex.h>
#include <stdint.h>
#include <stdlib.h>
#include <ti/condition.t.h>
#include <ti/field.t.h>
#include <ti/member.t.h>

int ti_condition_field_info_init(
        ti_field_t * field,
        const char * str,
        size_t n,
        ex_t * e);
int ti_condition_field_re_init(
        ti_field_t * field,
        const char * str,
        size_t n,
        ex_t * e);
int ti_condition_field_rel_init(
        ti_field_t * field,
        ti_field_t * ofield,
        ex_t * e);
int ti_condition_init_enum(ti_field_t * field, ti_member_t * member, ex_t * e);
int ti_condition_init_type(ti_field_t * field, ex_t * e);

void ti_condition_destroy(ti_condition_via_t condition, uint16_t spec);

#endif  /* TI_CONDITION_H_ */
