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
#include <ti/enum.h>
#include <util/strx.h>

/*
 * Returns 0 if the given value is valid for this field
 */
ti_spec_rval_enum ti__spec_check_val(uint16_t spec, ti_val_t * val)
{
    assert (~spec & TI_SPEC_NILLABLE);

    switch ((ti_spec_enum_t) spec)
    {
    case TI_SPEC_ANY:
        assert (0);
        return 0;
    case TI_SPEC_OBJECT:
        return ti_val_is_thing(val) ? 0 : TI_SPEC_RVAL_TYPE_ERROR;
    case TI_SPEC_RAW:
        return ti_val_is_raw(val) ? 0 : TI_SPEC_RVAL_TYPE_ERROR;
    case TI_SPEC_STR:
        return ti_val_is_str(val) ? 0 : TI_SPEC_RVAL_TYPE_ERROR;
    case TI_SPEC_UTF8:
        return !ti_val_is_str(val)
            ? TI_SPEC_RVAL_TYPE_ERROR
            : strx_is_utf8n(
                (const char *) ((ti_raw_t *) val)->data,
                ((ti_raw_t *) val)->n) ? 0 : TI_SPEC_RVAL_UTF8_ERROR;
    case TI_SPEC_BYTES:
        return ti_val_is_bytes(val) ? 0 : TI_SPEC_RVAL_TYPE_ERROR;
    case TI_SPEC_INT:
        return ti_val_is_int(val) ? 0 : TI_SPEC_RVAL_TYPE_ERROR;
    case TI_SPEC_UINT:
        return !ti_val_is_int(val)
            ? TI_SPEC_RVAL_TYPE_ERROR
            : VINT(val) < 0 ? TI_SPEC_RVAL_UINT_ERROR : 0;
    case TI_SPEC_PINT:
        return !ti_val_is_int(val)
            ? TI_SPEC_RVAL_TYPE_ERROR
            : VINT(val) <= 0 ? TI_SPEC_RVAL_PINT_ERROR : 0;
    case TI_SPEC_NINT:
        return !ti_val_is_int(val)
            ? TI_SPEC_RVAL_TYPE_ERROR
            : VINT(val) >= 0 ? TI_SPEC_RVAL_NINT_ERROR : 0;
    case TI_SPEC_FLOAT:
        return ti_val_is_float(val) ? 0 : TI_SPEC_RVAL_TYPE_ERROR;
    case TI_SPEC_NUMBER:
        return ti_val_is_number(val) ? 0 : TI_SPEC_RVAL_TYPE_ERROR;
    case TI_SPEC_BOOL:
        return ti_val_is_bool(val) ? 0 : TI_SPEC_RVAL_TYPE_ERROR;
    case TI_SPEC_ARR:
        return ti_val_is_array(val) ? 0 : TI_SPEC_RVAL_TYPE_ERROR;
    case TI_SPEC_SET:
        return ti_val_is_set(val) ? 0 : TI_SPEC_RVAL_TYPE_ERROR;
    }

    if (spec >= TI_ENUM_ID_FLAG)
        return ti_val_is_enum(val) && ti_venum_id((ti_venum_t *) val) == spec
                ? 0
                : TI_SPEC_RVAL_TYPE_ERROR;  /* TODO: maybe return enum error
                                                to make conversion possible */

    assert (spec < TI_SPEC_ANY);
    /*
     * Just compare the specification with the type since the nillable mask is
     * removed the specification
     */
    return ti_val_is_thing(val) && ((ti_thing_t *) val)->type_id == spec
            ? 0
            : TI_SPEC_RVAL_TYPE_ERROR;
}

_Bool ti__spec_maps_to_val(uint16_t spec, ti_val_t * val)
{
    assert (~spec & TI_SPEC_NILLABLE);

    switch ((ti_spec_enum_t) spec)
    {
    case TI_SPEC_ANY:
        assert (0);
        return true;
    case TI_SPEC_OBJECT:
        return ti_val_is_thing(val);
    case TI_SPEC_RAW:
        return ti_val_is_raw(val);
    case TI_SPEC_STR:
        return ti_val_is_str(val);
    case TI_SPEC_UTF8:
        return !ti_val_is_str(val)
            ? false
            : strx_is_utf8n(
                (const char *) ((ti_raw_t *) val)->data,
                ((ti_raw_t *) val)->n);
    case TI_SPEC_BYTES:
        return ti_val_is_bytes(val);
    case TI_SPEC_INT:
        return ti_val_is_int(val);
    case TI_SPEC_UINT:
        return !ti_val_is_int(val)
            ? false
            : VINT(val) >= 0;
    case TI_SPEC_PINT:
        return !ti_val_is_int(val)
            ? false
            : VINT(val) > 0;
    case TI_SPEC_NINT:
        return !ti_val_is_int(val)
            ? false
            : VINT(val) < 0;
    case TI_SPEC_FLOAT:
        return ti_val_is_float(val);
    case TI_SPEC_NUMBER:
        return ti_val_is_number(val);
    case TI_SPEC_BOOL:
        return ti_val_is_bool(val);
    case TI_SPEC_ARR:
        /* we can map a set to an array */
        return ti_val_is_array(val) || ti_val_is_set(val);
    case TI_SPEC_SET:
        return ti_val_is_set(val);
    }

    /* any *thing* can be mapped */
    return spec >= TI_ENUM_ID_FLAG
            ? ti_val_is_enum(val) && ti_venum_id((ti_venum_t *) val) == spec
            : ti_val_is_thing(val);
}

const char * ti__spec_approx_type_str(uint16_t spec)
{
    spec &= TI_SPEC_MASK_NILLABLE;
    switch ((ti_spec_enum_t) spec)
    {
    case TI_SPEC_ANY:           return "any";
    case TI_SPEC_OBJECT:        return TI_VAL_THING_S;
    case TI_SPEC_RAW:           return "raw";
    case TI_SPEC_STR:           return TI_VAL_STR_S;
    case TI_SPEC_UTF8:          return "utf8";
    case TI_SPEC_BYTES:         return TI_VAL_BYTES_S;
    case TI_SPEC_INT:           return TI_VAL_INT_S;
    case TI_SPEC_UINT:          return "uint";
    case TI_SPEC_PINT:          return "pint";
    case TI_SPEC_NINT:          return "nint";
    case TI_SPEC_FLOAT:         return TI_VAL_FLOAT_S;
    case TI_SPEC_NUMBER:        return "number";
    case TI_SPEC_BOOL:          return TI_VAL_BOOL_S;
    case TI_SPEC_ARR:           return TI_VAL_LIST_S;
    case TI_SPEC_SET:           return TI_VAL_SET_S;
    }
    return "thing";
}

ti_spec_mod_enum ti__spec_check_mod(uint16_t ospec, uint16_t nspec)
{
    if ((nspec & TI_SPEC_MASK_NILLABLE) == TI_SPEC_ANY)
        return TI_SPEC_MOD_SUCCESS;

    if ((ospec & TI_SPEC_NILLABLE) && (~nspec & TI_SPEC_NILLABLE))
        return TI_SPEC_MOD_NILLABLE_ERR;

    ospec &= TI_SPEC_MASK_NILLABLE;
    nspec &= TI_SPEC_MASK_NILLABLE;

    switch ((ti_spec_enum_t) nspec)
    {
    case TI_SPEC_ANY:
        return TI_SPEC_MOD_SUCCESS;
    case TI_SPEC_OBJECT:
        return ospec < TI_SPEC_ANY || ospec == TI_SPEC_OBJECT
                ? TI_SPEC_MOD_SUCCESS
                : TI_SPEC_MOD_ERR;
    case TI_SPEC_RAW:
        return (
            ospec == TI_SPEC_RAW ||
            ospec == TI_SPEC_STR ||
            ospec == TI_SPEC_UTF8 ||
            ospec == TI_SPEC_BYTES
        ) ? TI_SPEC_MOD_SUCCESS : TI_SPEC_MOD_ERR;
    case TI_SPEC_STR:
        return (
            ospec == TI_SPEC_STR ||
            ospec == TI_SPEC_UTF8
        )? TI_SPEC_MOD_SUCCESS : TI_SPEC_MOD_ERR;
    case TI_SPEC_UTF8:
    case TI_SPEC_BYTES:
    case TI_SPEC_PINT:
    case TI_SPEC_NINT:
    case TI_SPEC_FLOAT:
    case TI_SPEC_BOOL:
        return ospec == nspec ? TI_SPEC_MOD_SUCCESS : TI_SPEC_MOD_ERR;
    case TI_SPEC_UINT:
        return (
            ospec == TI_SPEC_UINT ||
            ospec == TI_SPEC_PINT
        ) ? TI_SPEC_MOD_SUCCESS : TI_SPEC_MOD_ERR;
    case TI_SPEC_INT:
        return (
            ospec == TI_SPEC_INT ||
            ospec == TI_SPEC_UINT ||
            ospec == TI_SPEC_PINT ||
            ospec == TI_SPEC_NINT
        ) ? TI_SPEC_MOD_SUCCESS : TI_SPEC_MOD_ERR;
    case TI_SPEC_NUMBER:
        return (
            ospec == TI_SPEC_NUMBER ||
            ospec == TI_SPEC_FLOAT ||
            ospec == TI_SPEC_INT ||
            ospec == TI_SPEC_UINT ||
            ospec == TI_SPEC_PINT ||
            ospec == TI_SPEC_NINT
        ) ? TI_SPEC_MOD_SUCCESS : TI_SPEC_MOD_ERR;
    case TI_SPEC_ARR:
    case TI_SPEC_SET:
        return ospec == nspec ? TI_SPEC_MOD_NESTED : TI_SPEC_MOD_ERR;
    }

    return ospec == nspec ? TI_SPEC_MOD_SUCCESS : TI_SPEC_MOD_ERR;
}
