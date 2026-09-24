#ifndef TI_OPR_OPRINC_H_
#define TI_OPR_OPRINC_H_

#define CAST_MAX 9223372036854775808.0

#include <assert.h>
#include <ti/datetime.h>
#include <ti/field.h>
#include <ti/opr.h>
#include <ti/raw.h>
#include <ti/val.inline.h>
#include <ti/vbool.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <ti/vset.h>
#include <util/logger.h>

#define TI_OPR_PERM(_a, _b) ((_a)->tp<<5|(_b)->tp)


typedef enum
{
    OPR_INT_INT             =TI_VAL_INT<<5|TI_VAL_INT,
    OPR_INT_FLOAT           =TI_VAL_INT<<5|TI_VAL_FLOAT,
    OPR_INT_BOOL            =TI_VAL_INT<<5|TI_VAL_BOOL,

    OPR_FLOAT_INT           =TI_VAL_FLOAT<<5|TI_VAL_INT,
    OPR_FLOAT_FLOAT         =TI_VAL_FLOAT<<5|TI_VAL_FLOAT,
    OPR_FLOAT_BOOL          =TI_VAL_FLOAT<<5|TI_VAL_BOOL,

    OPR_BOOL_INT            =TI_VAL_BOOL<<5|TI_VAL_INT,
    OPR_BOOL_FLOAT          =TI_VAL_BOOL<<5|TI_VAL_FLOAT,
    OPR_BOOL_BOOL           =TI_VAL_BOOL<<5|TI_VAL_BOOL,

    OPR_DATETIME_DATETIME   =TI_VAL_DATETIME<<5|TI_VAL_DATETIME,

    OPR_NAME_NAME           =TI_VAL_NAME<<5|TI_VAL_NAME,
    OPR_NAME_STR            =TI_VAL_NAME<<5|TI_VAL_STR,
    OPR_NAME_BYTES          =TI_VAL_NAME<<5|TI_VAL_BYTES,

    OPR_STR_NAME            =TI_VAL_STR<<5|TI_VAL_NAME,
    OPR_STR_STR             =TI_VAL_STR<<5|TI_VAL_STR,
    OPR_STR_BYTES           =TI_VAL_STR<<5|TI_VAL_BYTES,

    OPR_BYTES_NAME          =TI_VAL_BYTES<<5|TI_VAL_NAME,
    OPR_BYTES_STR           =TI_VAL_BYTES<<5|TI_VAL_STR,
    OPR_BYTES_BYTES         =TI_VAL_BYTES<<5|TI_VAL_BYTES,

    OPR_REGEX_REGEX         =TI_VAL_REGEX<<5|TI_VAL_REGEX,

    OPR_WRAP_WRAP           =TI_VAL_WRAP<<5|TI_VAL_WRAP,

    OPR_ARR_ARR             =TI_VAL_ARR<<5|TI_VAL_ARR,

    OPR_SET_SET             =TI_VAL_SET<<5|TI_VAL_SET,

    OPR_MEMBER_INT          =TI_VAL_MEMBER<<5|TI_VAL_INT,
    OPR_MEMBER_FLOAT        =TI_VAL_MEMBER<<5|TI_VAL_FLOAT,
    OPR_MEMBER_NAME         =TI_VAL_MEMBER<<5|TI_VAL_NAME,
    OPR_MEMBER_STR          =TI_VAL_MEMBER<<5|TI_VAL_STR,
    OPR_MEMBER_BYTES        =TI_VAL_MEMBER<<5|TI_VAL_BYTES,
    OPR_MEMBER_THING        =TI_VAL_MEMBER<<5|TI_VAL_THING,
    OPR_MEMBER_MEMBER       =TI_VAL_MEMBER<<5|TI_VAL_MEMBER,

    OPR_ANO_ANO             =TI_VAL_ANO<<5|TI_VAL_ANO,

    OPR_WANO_WANO           =TI_VAL_WANO<<5|TI_VAL_WANO,

    OPR_UUID_UUID           =TI_VAL_UUID<<5|TI_VAL_UUID,

} ti_opr_perm_t;

#endif  /* TI_OPR_OPRINC_H_ */
