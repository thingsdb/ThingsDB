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
    TI_SPEC_ANY=0x4000,
    TI_SPEC_OBJECT,                 /* this and below is valid for a set */
    TI_SPEC_RAW,
    TI_SPEC_UTF8,
    TI_SPEC_INT,
    TI_SPEC_UINT,
    TI_SPEC_FLOAT,
    TI_SPEC_NUMBER,
    TI_SPEC_BOOL,
    TI_SPEC_ARR,
    TI_SPEC_SET,
} ti_spec_enum_t;

typedef enum
{
    TI_SPEC_RET_SUCCESS,
    TI_SPEC_RET_TYPE_ERROR,
    TI_SPEC_RET_UTF8_ERROR,
    TI_SPEC_RET_UINT_ERROR,
} ti_spec_ret_enum_t;

ti_spec_ret_enum_t ti__spec_check(uint16_t spec, ti_val_t * val);

const char * ti__spec_type_str(uint16_t spec);

#endif  /* TI_SPEC_H_ */
