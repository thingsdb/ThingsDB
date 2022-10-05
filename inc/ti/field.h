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
#include <ti/spec.t.h>
#include <ti/query.t.h>
#include <util/vec.h>

void ti_field_init(void);
ti_field_map_t * ti_field_map_by_strn(const char * s, size_t n);
ti_field_map_t * ti_field_map_by_spec(const uint16_t spec);
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
        ex_t * e);
int ti_field_set_name(
        ti_field_t * field,
        const char * s,
        size_t n,
        ex_t * e);
int ti_field_del(ti_field_t * field);
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
ti_field_t * ti_field_by_strn_e(
        ti_type_t * type,
        const char * str,
        size_t n,
        ex_t * e);
int ti_field_init_things(ti_field_t * field, ti_val_t ** vaddr);
int ti_field_relation_check(
        ti_field_t * field,
        ti_field_t * ofield,
        vec_t * vars,
        ex_t * e);
int ti_field_relation_make(
        ti_field_t * field,
        ti_field_t * ofield,
        vec_t * vars);
int ti_field_vset_pre_assign(
        ti_field_t * field,
        imap_t * imap,
        ti_thing_t * parent,
        ex_t * e,
        _Bool do_type_check);

static inline ti_field_t * ti_field_by_name(ti_type_t * type, ti_name_t * name)
{
    for (vec_each(type->fields, ti_field_t, field))
        if (field->name == name)
            return field;
    return NULL;
}

static inline _Bool ti_field_has_relation(ti_field_t * field)
{
    /*
     * Field has a relation if at least a condition is set, and the field
     * is a non-nillable set or a nillable type.
     */
    return field->condition.none && (
            field->spec == TI_SPEC_SET ||
            (field->spec & TI_SPEC_MASK_NILLABLE) < TI_SPEC_ANY);
}

static inline uint8_t ti_field_deep(ti_field_t * field, uint8_t deep)
{
   return (
       (field->flags & TI_FIELD_FLAG_DEEP)
       ? deep
       : (field->flags & TI_FIELD_FLAG_MAX_DEEP)
       ? TI_MAX_DEEP-1
       : 0
   ) + (field->flags & TI_FIELD_FLAG_SAME_DEEP);
}

static inline int ti_field_ret_flags(ti_field_t * field)
{
   return field->flags & TI_FIELD_FLAG_NO_IDS;
}


#endif  /* TI_FIELD_H_ */
