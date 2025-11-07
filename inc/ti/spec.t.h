/*
 * ti/spec.t.h
 */
#ifndef TI_SPEC_T_H_
#define TI_SPEC_T_H_

#define TI_SPEC_NILLABLE        0x8000
#define TI_SPEC_MASK_NILLABLE   0x7fff


/*
 * Reserved for thing type: 0-0x3fff
 * Reserved for enum type: 0x6000-0x7fff
 */
typedef enum
{
    TI_SPEC_ANY=0x4000,     /* `any`    never together with nillable        */
    TI_SPEC_OBJECT,         /* `thing`  this and lower is valid for a set   */
    TI_SPEC_RAW,            /* `raw`                */
    TI_SPEC_STR,            /* `str`                */
    TI_SPEC_UTF8,           /* `utf8`               */
    TI_SPEC_BYTES,          /* `bytes`              */
    TI_SPEC_INT,            /* `int`                */
    TI_SPEC_UINT,           /* `uint`               */
    TI_SPEC_PINT,           /* `pint`               */
    TI_SPEC_NINT,           /* `nint`               */
    TI_SPEC_FLOAT,          /* `float`              */
    TI_SPEC_NUMBER,         /* `number`             */
    TI_SPEC_BOOL,           /* `bool`               */
    TI_SPEC_ARR,            /* `[..]`               */
    TI_SPEC_SET,            /* `{..}`               */
    TI_SPEC_DATETIME,       /* `datetime` (strict)  */
    TI_SPEC_TIMEVAL,        /* `timeval`            */
    TI_SPEC_REGEX,          /* `regex`              */
    TI_SPEC_CLOSURE,        /* `closure`            */
    TI_SPEC_ERROR,          /* `error`              */
    TI_SPEC_ROOM,           /* `room`               */
    TI_SPEC_TASK,           /* `task`               */
    TI_SPEC_EMAIL,          /* `email`              */
    TI_SPEC_URL,            /* `url`                */
    TI_SPEC_TEL,            /* `tel`                */
    TI_SPEC_ANO,            /* `ano`                */

    TI_SPEC_REMATCH=0x5000, /* `/.../`              */
    TI_SPEC_INT_RANGE,      /* `int<:>              */
    TI_SPEC_FLOAT_RANGE,    /* `float<:>            */
    TI_SPEC_STR_RANGE,      /* `str<:>              */
    TI_SPEC_UTF8_RANGE,     /* `utf8<:>             */
    TI_SPEC_TYPE,           /* `{...}               */
    TI_SPEC_ARR_TYPE,       /* `[{...}]             */
} ti_spec_enum_t;

typedef enum
{
    TI_SPEC_RVAL_SUCCESS,
    TI_SPEC_RVAL_TYPE_ERROR,
    TI_SPEC_RVAL_UTF8_ERROR,
    TI_SPEC_RVAL_UINT_ERROR,
    TI_SPEC_RVAL_PINT_ERROR,
    TI_SPEC_RVAL_NINT_ERROR,
    TI_SPEC_RVAL_EMAIL_ERROR,
    TI_SPEC_RVAL_URL_ERROR,
    TI_SPEC_RVAL_TEL_ERROR,
} ti_spec_rval_enum;

typedef enum
{
    TI_SPEC_MOD_SUCCESS,
    TI_SPEC_MOD_ERR,
    TI_SPEC_MOD_NILLABLE_ERR,
    TI_SPEC_MOD_NESTED,
} ti_spec_mod_enum;

#endif  /* TI_SPEC_T_H_ */
