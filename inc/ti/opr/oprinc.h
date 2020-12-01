#ifndef TI_OPR_OPRINC_H_
#define TI_OPR_OPRINC_H_

#define CAST_MAX 9223372036854775808.0

#include <assert.h>
#include <ti/opr.h>
#include <ti/raw.h>
#include <ti/val.inline.h>
#include <ti/vbool.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <ti/vset.h>
#include <ti/datetime.h>
#include <util/logger.h>

#define TI_OPR_PERM(_a, _b) ((_a)->tp<<4|(_b)->tp)


typedef enum
{
    OPR_NIL_NIL             =TI_VAL_NIL<<4|TI_VAL_NIL,
    OPR_NIL_INT             =TI_VAL_NIL<<4|TI_VAL_INT,
    OPR_NIL_FLOAT           =TI_VAL_NIL<<4|TI_VAL_FLOAT,
    OPR_NIL_BOOL            =TI_VAL_NIL<<4|TI_VAL_BOOL,
    OPR_NIL_DATETIME        =TI_VAL_NIL<<4|TI_VAL_DATETIME,
    OPR_NIL_NAME            =TI_VAL_NIL<<4|TI_VAL_NAME,
    OPR_NIL_STR             =TI_VAL_NIL<<4|TI_VAL_STR,
    OPR_NIL_BYTES           =TI_VAL_NIL<<4|TI_VAL_BYTES,
    OPR_NIL_REGEX           =TI_VAL_NIL<<4|TI_VAL_REGEX,
    OPR_NIL_THING           =TI_VAL_NIL<<4|TI_VAL_THING,
    OPR_NIL_WRAP            =TI_VAL_NIL<<4|TI_VAL_WRAP,
    OPR_NIL_ARR             =TI_VAL_NIL<<4|TI_VAL_ARR,
    OPR_NIL_SET             =TI_VAL_NIL<<4|TI_VAL_SET,
    OPR_NIL_CLOSURE         =TI_VAL_NIL<<4|TI_VAL_CLOSURE,
    OPR_NIL_ERROR           =TI_VAL_NIL<<4|TI_VAL_ERROR,
    OPR_NIL_ENUM            =TI_VAL_NIL<<4|TI_VAL_MEMBER,

    OPR_INT_NIL             =TI_VAL_INT<<4|TI_VAL_NIL,
    OPR_INT_INT             =TI_VAL_INT<<4|TI_VAL_INT,
    OPR_INT_FLOAT           =TI_VAL_INT<<4|TI_VAL_FLOAT,
    OPR_INT_BOOL            =TI_VAL_INT<<4|TI_VAL_BOOL,
    OPR_INT_DATETIME        =TI_VAL_INT<<4|TI_VAL_DATETIME,
    OPR_INT_NAME            =TI_VAL_INT<<4|TI_VAL_NAME,
    OPR_INT_STR             =TI_VAL_INT<<4|TI_VAL_STR,
    OPR_INT_BYTES           =TI_VAL_INT<<4|TI_VAL_BYTES,
    OPR_INT_REGEX           =TI_VAL_INT<<4|TI_VAL_REGEX,
    OPR_INT_THING           =TI_VAL_INT<<4|TI_VAL_THING,
    OPR_INT_WRAP            =TI_VAL_INT<<4|TI_VAL_WRAP,
    OPR_INT_ARR             =TI_VAL_INT<<4|TI_VAL_ARR,
    OPR_INT_SET             =TI_VAL_INT<<4|TI_VAL_SET,
    OPR_INT_CLOSURE         =TI_VAL_INT<<4|TI_VAL_CLOSURE,
    OPR_INT_ERROR           =TI_VAL_INT<<4|TI_VAL_ERROR,
    OPR_INT_ENUM            =TI_VAL_INT<<4|TI_VAL_MEMBER,

    OPR_FLOAT_NIL           =TI_VAL_FLOAT<<4|TI_VAL_NIL,
    OPR_FLOAT_INT           =TI_VAL_FLOAT<<4|TI_VAL_INT,
    OPR_FLOAT_FLOAT         =TI_VAL_FLOAT<<4|TI_VAL_FLOAT,
    OPR_FLOAT_BOOL          =TI_VAL_FLOAT<<4|TI_VAL_BOOL,
    OPR_FLOAT_DATETIME      =TI_VAL_FLOAT<<4|TI_VAL_DATETIME,
    OPR_FLOAT_NAME          =TI_VAL_FLOAT<<4|TI_VAL_NAME,
    OPR_FLOAT_STR           =TI_VAL_FLOAT<<4|TI_VAL_STR,
    OPR_FLOAT_BYTES         =TI_VAL_FLOAT<<4|TI_VAL_BYTES,
    OPR_FLOAT_REGEX         =TI_VAL_FLOAT<<4|TI_VAL_REGEX,
    OPR_FLOAT_THING         =TI_VAL_FLOAT<<4|TI_VAL_THING,
    OPR_FLOAT_WRAP          =TI_VAL_FLOAT<<4|TI_VAL_WRAP,
    OPR_FLOAT_ARR           =TI_VAL_FLOAT<<4|TI_VAL_ARR,
    OPR_FLOAT_SET           =TI_VAL_FLOAT<<4|TI_VAL_SET,
    OPR_FLOAT_CLOSURE       =TI_VAL_FLOAT<<4|TI_VAL_CLOSURE,
    OPR_FLOAT_ERROR         =TI_VAL_FLOAT<<4|TI_VAL_ERROR,
    OPR_FLOAT_ENUM          =TI_VAL_FLOAT<<4|TI_VAL_MEMBER,

    OPR_BOOL_NIL            =TI_VAL_BOOL<<4|TI_VAL_NIL,
    OPR_BOOL_INT            =TI_VAL_BOOL<<4|TI_VAL_INT,
    OPR_BOOL_FLOAT          =TI_VAL_BOOL<<4|TI_VAL_FLOAT,
    OPR_BOOL_BOOL           =TI_VAL_BOOL<<4|TI_VAL_BOOL,
    OPR_BOOL_DATETIME       =TI_VAL_BOOL<<4|TI_VAL_DATETIME,
    OPR_BOOL_NAME           =TI_VAL_BOOL<<4|TI_VAL_NAME,
    OPR_BOOL_STR            =TI_VAL_BOOL<<4|TI_VAL_STR,
    OPR_BOOL_BYTES          =TI_VAL_BOOL<<4|TI_VAL_BYTES,
    OPR_BOOL_REGEX          =TI_VAL_BOOL<<4|TI_VAL_REGEX,
    OPR_BOOL_THING          =TI_VAL_BOOL<<4|TI_VAL_THING,
    OPR_BOOL_WRAP           =TI_VAL_BOOL<<4|TI_VAL_WRAP,
    OPR_BOOL_ARR            =TI_VAL_BOOL<<4|TI_VAL_ARR,
    OPR_BOOL_SET            =TI_VAL_BOOL<<4|TI_VAL_SET,
    OPR_BOOL_CLOSURE        =TI_VAL_BOOL<<4|TI_VAL_CLOSURE,
    OPR_BOOL_ERROR          =TI_VAL_BOOL<<4|TI_VAL_ERROR,
    OPR_BOOL_ENUM           =TI_VAL_BOOL<<4|TI_VAL_MEMBER,

    OPR_DATETIME_NIL        =TI_VAL_DATETIME<<4|TI_VAL_NIL,
    OPR_DATETIME_INT        =TI_VAL_DATETIME<<4|TI_VAL_INT,
    OPR_DATETIME_FLOAT      =TI_VAL_DATETIME<<4|TI_VAL_FLOAT,
    OPR_DATETIME_BOOL       =TI_VAL_DATETIME<<4|TI_VAL_BOOL,
    OPR_DATETIME_DATETIME   =TI_VAL_DATETIME<<4|TI_VAL_DATETIME,
    OPR_DATETIME_NAME       =TI_VAL_DATETIME<<4|TI_VAL_NAME,
    OPR_DATETIME_STR        =TI_VAL_DATETIME<<4|TI_VAL_STR,
    OPR_DATETIME_BYTES      =TI_VAL_DATETIME<<4|TI_VAL_BYTES,
    OPR_DATETIME_REGEX      =TI_VAL_DATETIME<<4|TI_VAL_REGEX,
    OPR_DATETIME_THING      =TI_VAL_DATETIME<<4|TI_VAL_THING,
    OPR_DATETIME_WRAP       =TI_VAL_DATETIME<<4|TI_VAL_WRAP,
    OPR_DATETIME_ARR        =TI_VAL_DATETIME<<4|TI_VAL_ARR,
    OPR_DATETIME_SET        =TI_VAL_DATETIME<<4|TI_VAL_SET,
    OPR_DATETIME_CLOSURE    =TI_VAL_DATETIME<<4|TI_VAL_CLOSURE,
    OPR_DATETIME_ERROR      =TI_VAL_DATETIME<<4|TI_VAL_ERROR,
    OPR_DATETIME_ENUM       =TI_VAL_DATETIME<<4|TI_VAL_MEMBER,

    OPR_NAME_NIL            =TI_VAL_NAME<<4|TI_VAL_NIL,
    OPR_NAME_INT            =TI_VAL_NAME<<4|TI_VAL_INT,
    OPR_NAME_FLOAT          =TI_VAL_NAME<<4|TI_VAL_FLOAT,
    OPR_NAME_BOOL           =TI_VAL_NAME<<4|TI_VAL_BOOL,
    OPR_NAME_DATETIME       =TI_VAL_NAME<<4|TI_VAL_DATETIME,
    OPR_NAME_NAME           =TI_VAL_NAME<<4|TI_VAL_NAME,
    OPR_NAME_STR            =TI_VAL_NAME<<4|TI_VAL_STR,
    OPR_NAME_BYTES          =TI_VAL_NAME<<4|TI_VAL_BYTES,
    OPR_NAME_REGEX          =TI_VAL_NAME<<4|TI_VAL_REGEX,
    OPR_NAME_THING          =TI_VAL_NAME<<4|TI_VAL_THING,
    OPR_NAME_WRAP           =TI_VAL_NAME<<4|TI_VAL_WRAP,
    OPR_NAME_ARR            =TI_VAL_NAME<<4|TI_VAL_ARR,
    OPR_NAME_SET            =TI_VAL_NAME<<4|TI_VAL_SET,
    OPR_NAME_CLOSURE        =TI_VAL_NAME<<4|TI_VAL_CLOSURE,
    OPR_NAME_ERROR          =TI_VAL_NAME<<4|TI_VAL_ERROR,
    OPR_NAME_ENUM           =TI_VAL_NAME<<4|TI_VAL_MEMBER,

    OPR_STR_NIL             =TI_VAL_STR<<4|TI_VAL_NIL,
    OPR_STR_INT             =TI_VAL_STR<<4|TI_VAL_INT,
    OPR_STR_FLOAT           =TI_VAL_STR<<4|TI_VAL_FLOAT,
    OPR_STR_BOOL            =TI_VAL_STR<<4|TI_VAL_BOOL,
    OPR_STR_DATETIME        =TI_VAL_STR<<4|TI_VAL_DATETIME,
    OPR_STR_NAME            =TI_VAL_STR<<4|TI_VAL_NAME,
    OPR_STR_STR             =TI_VAL_STR<<4|TI_VAL_STR,
    OPR_STR_BYTES           =TI_VAL_STR<<4|TI_VAL_BYTES,
    OPR_STR_REGEX           =TI_VAL_STR<<4|TI_VAL_REGEX,
    OPR_STR_THING           =TI_VAL_STR<<4|TI_VAL_THING,
    OPR_STR_WRAP            =TI_VAL_STR<<4|TI_VAL_WRAP,
    OPR_STR_ARR             =TI_VAL_STR<<4|TI_VAL_ARR,
    OPR_STR_SET             =TI_VAL_STR<<4|TI_VAL_SET,
    OPR_STR_CLOSURE         =TI_VAL_STR<<4|TI_VAL_CLOSURE,
    OPR_STR_ERROR           =TI_VAL_STR<<4|TI_VAL_ERROR,
    OPR_STR_ENUM            =TI_VAL_STR<<4|TI_VAL_MEMBER,

    OPR_BYTES_NIL           =TI_VAL_BYTES<<4|TI_VAL_NIL,
    OPR_BYTES_INT           =TI_VAL_BYTES<<4|TI_VAL_INT,
    OPR_BYTES_FLOAT         =TI_VAL_BYTES<<4|TI_VAL_FLOAT,
    OPR_BYTES_BOOL          =TI_VAL_BYTES<<4|TI_VAL_BOOL,
    OPR_BYTES_DATETIME      =TI_VAL_BYTES<<4|TI_VAL_DATETIME,
    OPR_BYTES_NAME          =TI_VAL_BYTES<<4|TI_VAL_NAME,
    OPR_BYTES_STR           =TI_VAL_BYTES<<4|TI_VAL_STR,
    OPR_BYTES_BYTES         =TI_VAL_BYTES<<4|TI_VAL_BYTES,
    OPR_BYTES_REGEX         =TI_VAL_BYTES<<4|TI_VAL_REGEX,
    OPR_BYTES_THING         =TI_VAL_BYTES<<4|TI_VAL_THING,
    OPR_BYTES_WRAP          =TI_VAL_BYTES<<4|TI_VAL_WRAP,
    OPR_BYTES_ARR           =TI_VAL_BYTES<<4|TI_VAL_ARR,
    OPR_BYTES_SET           =TI_VAL_BYTES<<4|TI_VAL_SET,
    OPR_BYTES_CLOSURE       =TI_VAL_BYTES<<4|TI_VAL_CLOSURE,
    OPR_BYTES_ERROR         =TI_VAL_BYTES<<4|TI_VAL_ERROR,
    OPR_BYTES_ENUM          =TI_VAL_BYTES<<4|TI_VAL_MEMBER,

    OPR_REGEX_NIL           =TI_VAL_REGEX<<4|TI_VAL_NIL,
    OPR_REGEX_INT           =TI_VAL_REGEX<<4|TI_VAL_INT,
    OPR_REGEX_FLOAT         =TI_VAL_REGEX<<4|TI_VAL_FLOAT,
    OPR_REGEX_BOOL          =TI_VAL_REGEX<<4|TI_VAL_BOOL,
    OPR_REGEX_DATETIME      =TI_VAL_REGEX<<4|TI_VAL_DATETIME,
    OPR_REGEX_NAME          =TI_VAL_REGEX<<4|TI_VAL_NAME,
    OPR_REGEX_STR           =TI_VAL_REGEX<<4|TI_VAL_STR,
    OPR_REGEX_BYTES         =TI_VAL_REGEX<<4|TI_VAL_BYTES,
    OPR_REGEX_REGEX         =TI_VAL_REGEX<<4|TI_VAL_REGEX,
    OPR_REGEX_THING         =TI_VAL_REGEX<<4|TI_VAL_THING,
    OPR_REGEX_WRAP          =TI_VAL_REGEX<<4|TI_VAL_WRAP,
    OPR_REGEX_ARR           =TI_VAL_REGEX<<4|TI_VAL_ARR,
    OPR_REGEX_SET           =TI_VAL_REGEX<<4|TI_VAL_SET,
    OPR_REGEX_CLOSURE       =TI_VAL_REGEX<<4|TI_VAL_CLOSURE,
    OPR_REGEX_ERROR         =TI_VAL_REGEX<<4|TI_VAL_ERROR,
    OPR_REGEX_ENUM          =TI_VAL_REGEX<<4|TI_VAL_MEMBER,

    OPR_THING_NIL           =TI_VAL_THING<<4|TI_VAL_NIL,
    OPR_THING_INT           =TI_VAL_THING<<4|TI_VAL_INT,
    OPR_THING_FLOAT         =TI_VAL_THING<<4|TI_VAL_FLOAT,
    OPR_THING_BOOL          =TI_VAL_THING<<4|TI_VAL_BOOL,
    OPR_THING_DATETIME      =TI_VAL_THING<<4|TI_VAL_DATETIME,
    OPR_THING_NAME          =TI_VAL_THING<<4|TI_VAL_NAME,
    OPR_THING_STR           =TI_VAL_THING<<4|TI_VAL_STR,
    OPR_THING_BYTES         =TI_VAL_THING<<4|TI_VAL_BYTES,
    OPR_THING_REGEX         =TI_VAL_THING<<4|TI_VAL_REGEX,
    OPR_THING_THING         =TI_VAL_THING<<4|TI_VAL_THING,
    OPR_THING_WRAP          =TI_VAL_THING<<4|TI_VAL_WRAP,
    OPR_THING_ARR           =TI_VAL_THING<<4|TI_VAL_ARR,
    OPR_THING_SET           =TI_VAL_THING<<4|TI_VAL_SET,
    OPR_THING_CLOSURE       =TI_VAL_THING<<4|TI_VAL_CLOSURE,
    OPR_THING_ERROR         =TI_VAL_THING<<4|TI_VAL_ERROR,
    OPR_THING_ENUM          =TI_VAL_THING<<4|TI_VAL_MEMBER,

    OPR_WRAP_NIL            =TI_VAL_WRAP<<4|TI_VAL_NIL,
    OPR_WRAP_INT            =TI_VAL_WRAP<<4|TI_VAL_INT,
    OPR_WRAP_FLOAT          =TI_VAL_WRAP<<4|TI_VAL_FLOAT,
    OPR_WRAP_BOOL           =TI_VAL_WRAP<<4|TI_VAL_BOOL,
    OPR_WRAP_DATETIME       =TI_VAL_WRAP<<4|TI_VAL_DATETIME,
    OPR_WRAP_NAME           =TI_VAL_WRAP<<4|TI_VAL_NAME,
    OPR_WRAP_STR            =TI_VAL_WRAP<<4|TI_VAL_STR,
    OPR_WRAP_BYTES          =TI_VAL_WRAP<<4|TI_VAL_BYTES,
    OPR_WRAP_REGEX          =TI_VAL_WRAP<<4|TI_VAL_REGEX,
    OPR_WRAP_THING          =TI_VAL_WRAP<<4|TI_VAL_THING,
    OPR_WRAP_WRAP           =TI_VAL_WRAP<<4|TI_VAL_WRAP,
    OPR_WRAP_ARR            =TI_VAL_WRAP<<4|TI_VAL_ARR,
    OPR_WRAP_SET            =TI_VAL_WRAP<<4|TI_VAL_SET,
    OPR_WRAP_CLOSURE        =TI_VAL_WRAP<<4|TI_VAL_CLOSURE,
    OPR_WRAP_ERROR          =TI_VAL_WRAP<<4|TI_VAL_ERROR,
    OPR_WRAP_ENUM           =TI_VAL_WRAP<<4|TI_VAL_MEMBER,

    OPR_ARR_NIL             =TI_VAL_ARR<<4|TI_VAL_NIL,
    OPR_ARR_INT             =TI_VAL_ARR<<4|TI_VAL_INT,
    OPR_ARR_FLOAT           =TI_VAL_ARR<<4|TI_VAL_FLOAT,
    OPR_ARR_BOOL            =TI_VAL_ARR<<4|TI_VAL_BOOL,
    OPR_ARR_DATETIME        =TI_VAL_ARR<<4|TI_VAL_DATETIME,
    OPR_ARR_NAME            =TI_VAL_ARR<<4|TI_VAL_NAME,
    OPR_ARR_STR             =TI_VAL_ARR<<4|TI_VAL_STR,
    OPR_ARR_BYTES           =TI_VAL_ARR<<4|TI_VAL_BYTES,
    OPR_ARR_REGEX           =TI_VAL_ARR<<4|TI_VAL_REGEX,
    OPR_ARR_THING           =TI_VAL_ARR<<4|TI_VAL_THING,
    OPR_ARR_WRAP            =TI_VAL_ARR<<4|TI_VAL_WRAP,
    OPR_ARR_ARR             =TI_VAL_ARR<<4|TI_VAL_ARR,
    OPR_ARR_SET             =TI_VAL_ARR<<4|TI_VAL_SET,
    OPR_ARR_CLOSURE         =TI_VAL_ARR<<4|TI_VAL_CLOSURE,
    OPR_ARR_ERROR           =TI_VAL_ARR<<4|TI_VAL_ERROR,
    OPR_ARR_ENUM            =TI_VAL_ARR<<4|TI_VAL_MEMBER,

    OPR_SET_NIL             =TI_VAL_SET<<4|TI_VAL_NIL,
    OPR_SET_INT             =TI_VAL_SET<<4|TI_VAL_INT,
    OPR_SET_FLOAT           =TI_VAL_SET<<4|TI_VAL_FLOAT,
    OPR_SET_BOOL            =TI_VAL_SET<<4|TI_VAL_BOOL,
    OPR_SET_DATETIME        =TI_VAL_SET<<4|TI_VAL_DATETIME,
    OPR_SET_NAME            =TI_VAL_SET<<4|TI_VAL_NAME,
    OPR_SET_STR             =TI_VAL_SET<<4|TI_VAL_STR,
    OPR_SET_BYTES           =TI_VAL_SET<<4|TI_VAL_BYTES,
    OPR_SET_REGEX           =TI_VAL_SET<<4|TI_VAL_REGEX,
    OPR_SET_THING           =TI_VAL_SET<<4|TI_VAL_THING,
    OPR_SET_WRAP            =TI_VAL_SET<<4|TI_VAL_WRAP,
    OPR_SET_ARR             =TI_VAL_SET<<4|TI_VAL_ARR,
    OPR_SET_SET             =TI_VAL_SET<<4|TI_VAL_SET,
    OPR_SET_CLOSURE         =TI_VAL_SET<<4|TI_VAL_CLOSURE,
    OPR_SET_ERROR           =TI_VAL_SET<<4|TI_VAL_ERROR,
    OPR_SET_ENUM            =TI_VAL_SET<<4|TI_VAL_MEMBER,

    OPR_CLOSURE_NIL         =TI_VAL_CLOSURE<<4|TI_VAL_NIL,
    OPR_CLOSURE_INT         =TI_VAL_CLOSURE<<4|TI_VAL_INT,
    OPR_CLOSURE_FLOAT       =TI_VAL_CLOSURE<<4|TI_VAL_FLOAT,
    OPR_CLOSURE_BOOL        =TI_VAL_CLOSURE<<4|TI_VAL_BOOL,
    OPR_CLOSURE_DATETIME    =TI_VAL_CLOSURE<<4|TI_VAL_DATETIME,
    OPR_CLOSURE_NAME        =TI_VAL_CLOSURE<<4|TI_VAL_NAME,
    OPR_CLOSURE_STR         =TI_VAL_CLOSURE<<4|TI_VAL_STR,
    OPR_CLOSURE_BYTES       =TI_VAL_CLOSURE<<4|TI_VAL_BYTES,
    OPR_CLOSURE_REGEX       =TI_VAL_CLOSURE<<4|TI_VAL_REGEX,
    OPR_CLOSURE_THING       =TI_VAL_CLOSURE<<4|TI_VAL_THING,
    OPR_CLOSURE_WRAP        =TI_VAL_CLOSURE<<4|TI_VAL_WRAP,
    OPR_CLOSURE_ARR         =TI_VAL_CLOSURE<<4|TI_VAL_ARR,
    OPR_CLOSURE_SET         =TI_VAL_CLOSURE<<4|TI_VAL_SET,
    OPR_CLOSURE_CLOSURE     =TI_VAL_CLOSURE<<4|TI_VAL_CLOSURE,
    OPR_CLOSURE_ERROR       =TI_VAL_CLOSURE<<4|TI_VAL_ERROR,
    OPR_CLOSURE_ENUM        =TI_VAL_CLOSURE<<4|TI_VAL_MEMBER,

    OPR_ERROR_NIL           =TI_VAL_ERROR<<4|TI_VAL_NIL,
    OPR_ERROR_INT           =TI_VAL_ERROR<<4|TI_VAL_INT,
    OPR_ERROR_FLOAT         =TI_VAL_ERROR<<4|TI_VAL_FLOAT,
    OPR_ERROR_BOOL          =TI_VAL_ERROR<<4|TI_VAL_BOOL,
    OPR_ERROR_DATETIME      =TI_VAL_ERROR<<4|TI_VAL_DATETIME,
    OPR_ERROR_NAME          =TI_VAL_ERROR<<4|TI_VAL_NAME,
    OPR_ERROR_STR           =TI_VAL_ERROR<<4|TI_VAL_STR,
    OPR_ERROR_BYTES         =TI_VAL_ERROR<<4|TI_VAL_BYTES,
    OPR_ERROR_REGEX         =TI_VAL_ERROR<<4|TI_VAL_REGEX,
    OPR_ERROR_THING         =TI_VAL_ERROR<<4|TI_VAL_THING,
    OPR_ERROR_WRAP          =TI_VAL_ERROR<<4|TI_VAL_WRAP,
    OPR_ERROR_ARR           =TI_VAL_ERROR<<4|TI_VAL_ARR,
    OPR_ERROR_SET           =TI_VAL_ERROR<<4|TI_VAL_SET,
    OPR_ERROR_CLOSURE       =TI_VAL_ERROR<<4|TI_VAL_CLOSURE,
    OPR_ERROR_ERROR         =TI_VAL_ERROR<<4|TI_VAL_ERROR,
    OPR_ERROR_ENUM          =TI_VAL_ERROR<<4|TI_VAL_MEMBER,

    OPR_ENUM_NIL            =TI_VAL_MEMBER<<4|TI_VAL_NIL,
    OPR_ENUM_INT            =TI_VAL_MEMBER<<4|TI_VAL_INT,
    OPR_ENUM_FLOAT          =TI_VAL_MEMBER<<4|TI_VAL_FLOAT,
    OPR_ENUM_BOOL           =TI_VAL_MEMBER<<4|TI_VAL_BOOL,
    OPR_ENUM_DATETIME       =TI_VAL_MEMBER<<4|TI_VAL_DATETIME,
    OPR_ENUM_NAME           =TI_VAL_MEMBER<<4|TI_VAL_NAME,
    OPR_ENUM_STR            =TI_VAL_MEMBER<<4|TI_VAL_STR,
    OPR_ENUM_BYTES          =TI_VAL_MEMBER<<4|TI_VAL_BYTES,
    OPR_ENUM_REGEX          =TI_VAL_MEMBER<<4|TI_VAL_REGEX,
    OPR_ENUM_THING          =TI_VAL_MEMBER<<4|TI_VAL_THING,
    OPR_ENUM_WRAP           =TI_VAL_MEMBER<<4|TI_VAL_WRAP,
    OPR_ENUM_ARR            =TI_VAL_MEMBER<<4|TI_VAL_ARR,
    OPR_ENUM_SET            =TI_VAL_MEMBER<<4|TI_VAL_SET,
    OPR_ENUM_CLOSURE        =TI_VAL_MEMBER<<4|TI_VAL_CLOSURE,
    OPR_ENUM_ERROR          =TI_VAL_MEMBER<<4|TI_VAL_ERROR,
    OPR_ENUM_ENUM           =TI_VAL_MEMBER<<4|TI_VAL_MEMBER,
} ti_opr_perm_t;

#endif  /* TI_OPR_OPRINC_H_ */
