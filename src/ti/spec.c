/*
 * ti/type.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <ti/type.h>
#include <ti/prop.h>
#include <ti/vint.h>
#include <ti/spec.h>
#include <util/strx.h>

/*
 * Returns 0 if the given value is valid for this field
 */
ti_spec_ret_enum_t ti__spec_check(uint16_t spec, ti_val_t * val)
{
    assert (~spec & TI_SPEC_NILLABLE);

    switch ((ti_spec_enum_t) spec)
    {
    case TI_SPEC_ANY:
        assert (0);
        return 0;
    case TI_SPEC_OBJECT:
        return ti_val_is_thing(val) ? 0 : TI_SPEC_RET_TYPE_ERROR;
    case TI_SPEC_RAW:
        return ti_val_is_raw(val) ? 0 : TI_SPEC_RET_TYPE_ERROR;
    case TI_SPEC_UTF8:
        return !ti_val_is_raw(val)
            ? TI_SPEC_RET_TYPE_ERROR
            : strx_is_utf8n(
                (const char *) ((ti_raw_t *) val)->data,
                ((ti_raw_t *) val)->n) ? 0 : TI_SPEC_RET_UTF8_ERROR;
    case TI_SPEC_INT:
        return ti_val_is_int(val) ? 0 : TI_SPEC_RET_TYPE_ERROR;
    case TI_SPEC_UINT:
        return !ti_val_is_int(val)
            ? TI_SPEC_RET_TYPE_ERROR
            : ((ti_vint_t *) val)->int_ < 0 ? TI_SPEC_RET_UINT_ERROR : 0;
    case TI_SPEC_FLOAT:
        return ti_val_is_float(val) ? 0 : TI_SPEC_RET_TYPE_ERROR;
    case TI_SPEC_NUMBER:
        return ti_val_is_number(val) ? 0 : TI_SPEC_RET_TYPE_ERROR;
    case TI_SPEC_BOOL:
        return ti_val_is_bool(val) ? 0 : TI_SPEC_RET_TYPE_ERROR;
    case TI_SPEC_ARR:
        return ti_val_is_array(val) ? 0 : TI_SPEC_RET_TYPE_ERROR;
    case TI_SPEC_SET:
        return ti_val_is_set(val) ? 0 : TI_SPEC_RET_TYPE_ERROR;
    }

    assert (spec < TI_SPEC_ANY);
    /*
     * Just compare the specification with the type since the nillable mask is
     * removed the specification
     */
    return ti_val_is_thing(val) && ((ti_thing_t *) val)->type_id == spec
            ? 0
            : TI_SPEC_RET_TYPE_ERROR;
}

const char * ti__spec_approx_type_str(uint16_t spec)
{
    spec &= TI_SPEC_MASK_NILLABLE;
    switch ((ti_spec_enum_t) spec)
    {
    case TI_SPEC_ANY:           return "any";
    case TI_SPEC_OBJECT:        return TI_VAL_THING_S;
    case TI_SPEC_RAW:
    case TI_SPEC_UTF8:          return TI_VAL_RAW_S;
    case TI_SPEC_INT:
    case TI_SPEC_UINT:          return TI_VAL_INT_S;
    case TI_SPEC_FLOAT:         return TI_VAL_FLOAT_S;
    case TI_SPEC_NUMBER:        return "number";
    case TI_SPEC_BOOL:          return TI_VAL_BOOL_S;
    case TI_SPEC_ARR:           return TI_VAL_ARR_S;
    case TI_SPEC_SET:           return TI_VAL_SET_S;
    }

    return "thing";
}
