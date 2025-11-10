/*
 * ti/spec.h
 */
#ifndef TI_SPEC_H_
#define TI_SPEC_H_

#include <ex.h>
#include <inttypes.h>
#include <ti/condition.t.h>
#include <ti/raw.t.h>
#include <ti/spec.t.h>
#include <ti/val.t.h>
#include <ti/field.t.h>

ti_spec_rval_enum ti__spec_check_nested_val(uint16_t spec, ti_val_t * val);
_Bool ti__spec_maps_to_nested_val(ti_field_t * field, ti_val_t * val);

const char * ti_spec_approx_type_str(uint16_t spec);
ti_spec_mod_enum ti_spec_check_mod(
        uint16_t ospec,
        uint16_t nspec,
        ti_condition_via_t ocondition,
        ti_condition_via_t ncondition);
_Bool ti_spec_is_reserved(register const char * s, register size_t n);
int ti_spec_from_raw(
        uint16_t * spec,
        ti_raw_t * raw,
        ti_collection_t * collection,
        ex_t * e);
ti_raw_t * ti_spec_raw(uint16_t spec, ti_collection_t * collection);

#endif  /* TI_SPEC_H_ */
