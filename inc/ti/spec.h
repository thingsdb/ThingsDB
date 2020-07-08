/*
 * ti/spec.h
 */
#ifndef TI_SPEC_H_
#define TI_SPEC_H_

#include <inttypes.h>
#include <ti/spec.t.h>
#include <ti/val.t.h>
#include <ti/condition.t.h>

ti_spec_rval_enum ti__spec_check_nested_val(uint16_t spec, ti_val_t * val);
_Bool ti__spec_maps_to_nested_val(uint16_t spec, ti_val_t * val);
const char * ti__spec_approx_type_str(uint16_t spec);
ti_spec_mod_enum ti__spec_check_mod(
        uint16_t ospec,
        uint16_t nspec,
        ti_condition_via_t ocondition,
        ti_condition_via_t ncondition);
_Bool ti_spec_is_reserved(register const char * s, register size_t n);

#endif  /* TI_SPEC_H_ */
