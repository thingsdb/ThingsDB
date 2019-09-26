/*
 * ti/spec.h
 */
#ifndef TI_SPEC_H_
#define TI_SPEC_H_

#include <ti/val.h>

#define TI_SPEC_NILLABLE        0x8000
#define TI_SPEC_MASK_NILLABLE   0x7fff

/*
 * Reserved for thing type: 0-0x3FFF
 */
typedef enum
{
    TI_SPEC_ANY=0x4000,     /* `any`    never together with nillable        */
    TI_SPEC_OBJECT,         /* `thing`  this and lower is valid for a set   */
    TI_SPEC_RAW,            /* `raw`                */
    TI_SPEC_UTF8,           /* `utf8` or `str`      */
    TI_SPEC_INT,            /* `int`                */
    TI_SPEC_UINT,           /* `uint`               */
    TI_SPEC_FLOAT,          /* `float`              */
    TI_SPEC_NUMBER,         /* `number`             */
    TI_SPEC_BOOL,           /* `bool`               */
    TI_SPEC_ARR,            /* `[..]`               */
    TI_SPEC_SET,            /* `{..}`               */
} ti_spec_enum_t;

typedef enum
{
    TI_SPEC_RVAL_SUCCESS,
    TI_SPEC_RVAL_TYPE_ERROR,
    TI_SPEC_RVAL_UTF8_ERROR,
    TI_SPEC_RVAL_UINT_ERROR,
} ti_spec_rval_enum;

ti_spec_rval_enum ti__spec_check_val(uint16_t spec, ti_val_t * val);

const char * ti__spec_approx_type_str(uint16_t spec);

#endif  /* TI_SPEC_H_ */
