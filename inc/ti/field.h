/*
 * ti/field.h
 */
#ifndef TI_FIELD_H_
#define TI_FIELD_H_

#include <ex.h>
#include <inttypes.h>
#include <ti/field.t.h>
#include <ti/name.t.h>
#include <ti/raw.t.h>
#include <ti/thing.t.h>
#include <ti/val.t.h>
#include <util/vec.h>

ti_field_t * ti_field_create(
        ti_name_t * name,
        ti_raw_t * spec_raw,
        ti_type_t * type,
        ex_t * e);
ti_field_t * ti_field_as_new(ti_field_t * field, ti_raw_t * spec_raw, ex_t * e);
void ti_field_replace(ti_field_t * field, ti_field_t ** with_field);
int ti_field_mod_force(ti_field_t * field, ti_raw_t * spec_raw, ex_t * e);
int ti_field_mod(
        ti_field_t * field,
        ti_raw_t * spec_raw,
        vec_t * vars,
        ex_t * e);
int ti_field_set_name(
        ti_field_t * field,
        vec_t * vars,
        const char * s,
        size_t n,
        ex_t * e);
int ti_field_del(ti_field_t * field, uint64_t ev_id);
void ti_field_remove(ti_field_t * field);
void ti_field_destroy(ti_field_t * field);
void ti_field_destroy_dep(ti_field_t * field);
int ti_field_make_assignable(
        ti_field_t * field,
        ti_val_t ** val,
        ti_thing_t * parent,  /* may be NULL */
        ex_t * e);
_Bool ti_field_maps_to_val(ti_field_t * field, ti_val_t * val);
_Bool ti_field_maps_to_field(ti_field_t * t_field, ti_field_t * f_field);
ti_field_t * ti_field_by_name(ti_type_t * type, ti_name_t * name);
ti_field_t * ti_field_by_strn_e(
        ti_type_t * type,
        const char * str,
        size_t n,
        ex_t * e);
int ti_field_init_things(ti_field_t * field, ti_val_t ** vaddr, uint64_t ev_id);
ti_val_t * ti_field_dval(ti_field_t * field);

#endif  /* TI_FIELD_H_ */
