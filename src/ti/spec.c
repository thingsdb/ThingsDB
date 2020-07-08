/*
 * ti/type.c
 */
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ti/enum.h>
#include <ti/member.inline.h>
#include <ti/prop.h>
#include <ti/spec.h>
#include <ti/spec.inline.h>
#include <ti/type.h>
#include <ti/val.inline.h>
#include <ti/vint.h>
#include <util/strx.h>

/*
 * The following `enum` and `asso_values` are generated using `gperf` and
 * the utility `pcregrep` to generate the input.
 *
 * Command:
 *
 *    pcregrep -o1 ' :: `(\w+)' spec.c | gperf -E -k '*,1,$' -m 200

  :: `str`
  :: `utf8`
  :: `raw`
  :: `bytes`
  :: `bool`
  :: `int`
  :: `uint`
  :: `pint`
  :: `nint`
  :: `float`
  :: `number`
  :: `thing`
  :: `any`
  :: `closure`
  :: `regex`
  :: `error`
  :: `set`
  :: `tuple`
  :: `list`
  :: `nil`

 */

enum
{
    TOTAL_KEYWORDS = 20,
    MIN_WORD_LENGTH = 3,
    MAX_WORD_LENGTH = 7,
    MIN_HASH_VALUE = 6,
    MAX_HASH_VALUE = 25
};

static inline unsigned int spec__hash(
        register const char * s,
        register size_t n)
{
    static unsigned char asso_values[] =
    {
        26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
        26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
        26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
        26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
        26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
        26, 26, 26, 26, 26, 26, 14, 26, 26, 26,
        26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
        26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
        26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
        26, 26, 26, 26, 26, 26, 26,  4,  0,  9,
        26,  0,  4,  8,  1,  3, 26, 26,  0, 11,
        2,  7,  1, 26,  1,  0,  3,  0, 26, 11,
        7,  8, 26, 26, 26, 26, 26, 26, 26, 26,
        26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
        26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
        26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
        26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
        26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
        26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
        26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
        26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
        26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
        26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
        26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
        26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
        26, 26, 26, 26, 26, 26
    };

    register unsigned int hval = n;

    switch (hval)
    {
        default:
            hval += asso_values[(unsigned char)s[6]];
            /*fall through*/
        case 6:
            hval += asso_values[(unsigned char)s[5]];
            /*fall through*/
        case 5:
            hval += asso_values[(unsigned char)s[4]];
            /*fall through*/
        case 4:
            hval += asso_values[(unsigned char)s[3]];
            /*fall through*/
        case 3:
            hval += asso_values[(unsigned char)s[2]];
            /*fall through*/
        case 2:
            hval += asso_values[(unsigned char)s[1]];
            /*fall through*/
        case 1:
            hval += asso_values[(unsigned char)s[0]];
            break;
    }
    return hval;
}

_Bool ti_spec_is_reserved(register const char * s, register size_t n)
{
    static const char * wordlist[] =
    {
        "", "", "", "", "", "",
        "set",
        "str",
        "nil",
        "tuple",
        "list",
        "int",
        "uint",
        "pint",
        "nint",
        "error",
        "bytes",
        "any",
        "bool",
        "raw",
        "number",
        "regex",
        "thing",
        "float",
        "closure",
        "utf8"
    };

    if (n <= MAX_WORD_LENGTH && n >= MIN_WORD_LENGTH)
    {
        register unsigned int key = spec__hash (s, n);
        if (key <= MAX_HASH_VALUE)
        {
            register const char * ws = wordlist[key];

            if (strlen(ws) == n && !memcmp(s, ws, n))
                return true;
        }
    }
    return false;
}

/*
 * Returns 0 if the given value is valid for this field
 */
ti_spec_rval_enum ti__spec_check_nested_val(uint16_t spec, ti_val_t * val)
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

    case TI_SPEC_REMATCH:
    case TI_SPEC_INT_RANGE:
    case TI_SPEC_FLOAT_RANGE:
    case TI_SPEC_STR_RANGE:
        assert (0);
        return false;
    }

    if (spec >= TI_ENUM_ID_FLAG)
        return ti_spec_enum_eq_to_val(spec, val) ? 0 : TI_SPEC_RVAL_TYPE_ERROR;

    assert (spec < TI_SPEC_ANY);
    /*
     * Just compare the specification with the type since the nillable mask is
     * removed the specification
     */
    return ti_val_is_thing(val) && ((ti_thing_t *) val)->type_id == spec
            ? 0
            : TI_SPEC_RVAL_TYPE_ERROR;
}

_Bool ti__spec_maps_to_nested_val(uint16_t spec, ti_val_t * val)
{
    assert (~spec & TI_SPEC_NILLABLE);

    if (spec >= TI_ENUM_ID_FLAG)
        return ti_spec_enum_eq_to_val(spec, val);

    if (ti_val_is_member(val))
        val = VMEMBER(val);

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
        return ti_val_is_str(val) && strx_is_utf8n(
                (const char *) ((ti_raw_t *) val)->data,
                ((ti_raw_t *) val)->n);
    case TI_SPEC_BYTES:
        return ti_val_is_bytes(val);
    case TI_SPEC_INT:
        return ti_val_is_int(val);
    case TI_SPEC_UINT:
        return ti_val_is_int(val) && VINT(val) >= 0;
    case TI_SPEC_PINT:
        return ti_val_is_int(val) && VINT(val) > 0;
    case TI_SPEC_NINT:
        return ti_val_is_int(val) && VINT(val) < 0;
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
    case TI_SPEC_REMATCH:
    case TI_SPEC_INT_RANGE:
    case TI_SPEC_FLOAT_RANGE:
    case TI_SPEC_STR_RANGE:
        assert (0);  /* only nested so conditions are not possible */
        return false;
    }

    /* any *thing* can be mapped */
    return ti_val_is_thing(val);
}

const char * ti__spec_approx_type_str(uint16_t spec)
{
    spec &= TI_SPEC_MASK_NILLABLE;
    switch ((ti_spec_enum_t) spec)
    {
    case TI_SPEC_ANY:           return "any";
    case TI_SPEC_OBJECT:        return "thing";
    case TI_SPEC_RAW:           return "raw";
    case TI_SPEC_REMATCH:
    case TI_SPEC_STR_RANGE:
    case TI_SPEC_STR:           return "str";
    case TI_SPEC_UTF8:          return "utf8";
    case TI_SPEC_BYTES:         return "bytes";
    case TI_SPEC_INT_RANGE:
    case TI_SPEC_INT:           return "int";
    case TI_SPEC_UINT:          return "uint";
    case TI_SPEC_PINT:          return "pint";
    case TI_SPEC_NINT:          return "nint";
    case TI_SPEC_FLOAT_RANGE:
    case TI_SPEC_FLOAT:         return "float";
    case TI_SPEC_NUMBER:        return "number";
    case TI_SPEC_BOOL:          return "bool";
    case TI_SPEC_ARR:           return "list";
    case TI_SPEC_SET:           return "set";
    }
    return spec < TI_SPEC_ANY ? "thing" : "enum";
}

ti_spec_mod_enum ti__spec_check_mod(
        uint16_t ospec,
        uint16_t nspec,
        ti_condition_via_t ocondition,
        ti_condition_via_t ncondition)
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
            ospec == TI_SPEC_BYTES ||
            ospec == TI_SPEC_REMATCH ||
            ospec == TI_SPEC_STR_RANGE
        ) ? TI_SPEC_MOD_SUCCESS : TI_SPEC_MOD_ERR;
    case TI_SPEC_STR:
        return (
            ospec == TI_SPEC_STR ||
            ospec == TI_SPEC_UTF8 ||
            ospec == TI_SPEC_REMATCH ||
            ospec == TI_SPEC_STR_RANGE
        ) ? TI_SPEC_MOD_SUCCESS : TI_SPEC_MOD_ERR;
    case TI_SPEC_UTF8:
    case TI_SPEC_BYTES:
        return ospec == nspec ? TI_SPEC_MOD_SUCCESS : TI_SPEC_MOD_ERR;
    case TI_SPEC_INT:
        return (
            ospec == TI_SPEC_INT ||
            ospec == TI_SPEC_UINT ||
            ospec == TI_SPEC_PINT ||
            ospec == TI_SPEC_NINT ||
            ospec == TI_SPEC_INT_RANGE
        ) ? TI_SPEC_MOD_SUCCESS : TI_SPEC_MOD_ERR;
    case TI_SPEC_UINT:
        return (
            ospec == TI_SPEC_UINT ||
            ospec == TI_SPEC_PINT ||
            (ospec == TI_SPEC_INT_RANGE && ocondition.irange->mi >= 0)
        ) ? TI_SPEC_MOD_SUCCESS : TI_SPEC_MOD_ERR;
    case TI_SPEC_PINT:
        return (
            ospec == TI_SPEC_PINT ||
            (ospec == TI_SPEC_INT_RANGE && ocondition.irange->mi >= 1)
        ) ? TI_SPEC_MOD_SUCCESS : TI_SPEC_MOD_ERR;
    case TI_SPEC_NINT:
        return (
            ospec == TI_SPEC_NINT ||
            (ospec == TI_SPEC_INT_RANGE && ocondition.irange->ma <= -1)
        ) ? TI_SPEC_MOD_SUCCESS : TI_SPEC_MOD_ERR;
    case TI_SPEC_FLOAT:
        return (
            ospec == TI_SPEC_FLOAT ||
            ospec == TI_SPEC_FLOAT_RANGE
        ) ? TI_SPEC_MOD_SUCCESS : TI_SPEC_MOD_ERR;
    case TI_SPEC_NUMBER:
        return (
            ospec == TI_SPEC_NUMBER ||
            ospec == TI_SPEC_FLOAT ||
            ospec == TI_SPEC_INT ||
            ospec == TI_SPEC_UINT ||
            ospec == TI_SPEC_PINT ||
            ospec == TI_SPEC_NINT ||
            ospec == TI_SPEC_INT_RANGE ||
            ospec == TI_SPEC_FLOAT_RANGE
        ) ? TI_SPEC_MOD_SUCCESS : TI_SPEC_MOD_ERR;
    case TI_SPEC_BOOL:
        return ospec == nspec ? TI_SPEC_MOD_SUCCESS : TI_SPEC_MOD_ERR;
    case TI_SPEC_ARR:
    case TI_SPEC_SET:
        return ospec == nspec ? TI_SPEC_MOD_NESTED : TI_SPEC_MOD_ERR;
    case TI_SPEC_REMATCH:
        return TI_SPEC_MOD_ERR;
    case TI_SPEC_INT_RANGE:
        return (
            ospec == TI_SPEC_INT_RANGE &&
            ocondition.irange->mi >= ncondition.irange->mi &&
            ocondition.irange->ma <= ncondition.irange->ma
        ) ? TI_SPEC_MOD_SUCCESS : TI_SPEC_MOD_ERR;
    case TI_SPEC_FLOAT_RANGE:
        return (
            ospec == TI_SPEC_FLOAT_RANGE &&
            ocondition.drange->mi >= ncondition.drange->mi &&
            ocondition.drange->ma <= ncondition.drange->ma
        ) ? TI_SPEC_MOD_SUCCESS : TI_SPEC_MOD_ERR;
    case TI_SPEC_STR_RANGE:
        return (
            ospec == TI_SPEC_STR_RANGE &&
            ocondition.srange->mi >= ncondition.srange->mi &&
            ocondition.srange->ma <= ncondition.srange->ma
        ) ? TI_SPEC_MOD_SUCCESS : TI_SPEC_MOD_ERR;
    }

    return ospec == nspec ? TI_SPEC_MOD_SUCCESS : TI_SPEC_MOD_ERR;
}
