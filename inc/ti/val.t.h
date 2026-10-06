/*
 * ti/val.t.h
 */
#ifndef TI_VAL_T_H_
#define TI_VAL_T_H_

#define VAL__CAST_MAX 9223372036854775808.0

#define TI_VAL_NIL_S        "nil"
#define TI_VAL_INT_S        "int"
#define TI_VAL_FLOAT_S      "float"
#define TI_VAL_BOOL_S       "bool"
#define TI_VAL_MPDATA_S     "mpdata"
#define TI_VAL_STR_S        "str"
#define TI_VAL_BYTES_S      "bytes"
#define TI_VAL_REGEX_S      "regex"
#define TI_VAL_THING_S      "thing"
#define TI_VAL_LIST_S       "list"
#define TI_VAL_TUPLE_S      "tuple"
#define TI_VAL_ROOM_S       "room"
#define TI_VAL_SET_S        "set"
#define TI_VAL_CLOSURE_S    "closure"
#define TI_VAL_ERROR_S      "error"
#define TI_VAL_DATETIME_S   "datetime"
#define TI_VAL_TIMEVAL_S    "timeval"
#define TI_VAL_FUTURE_S     "future"
#define TI_VAL_MODULE_S     "module"
#define TI_VAL_TASK_S       "task"
#define TI_VAL_ANO_S        "<anonymous>"
#define TI_VAL_WANO_S       "<<anonymous>>"

#define TI_KIND_S_INSTANCE  "."     /* Internally, New typed thing */
#define TI_KIND_S_OBJECT    ","     /* Internally, New thing */
#define TI_KIND_S_THING     "#"     /* Externally, Thing */
#define TI_KIND_S_SET       "$"     /* Internally, Set */
#define TI_KIND_S_ERROR     "!"     /* Internally, Error */
#define TI_KIND_S_WRAP      "&"     /* Internally, Wrapped thing */
#define TI_KIND_S_WANO      " "     /* Internally, Wrapped ano thing */
#define TI_KIND_S_MEMBER    "%%"    /* Internally, Enum member */
#define TI_KIND_S_DATETIME  "'"     /* Internally, Date/Time */
#define TI_KIND_S_TIMEVAL   "\""    /* Internally, Time value */
#define TI_KIND_S_CLOSURE_OBSOLETE_   "/"
#define TI_KIND_S_REGEX_OBSOLETE_     "*"

/*
 * Be careful when changing the order in the enumerator.
 * The ti_forloop_t also depends on the order in this enumerator;
 */
typedef enum
{
    TI_VAL_NIL,
    TI_VAL_INT,
    TI_VAL_FLOAT,
    TI_VAL_BOOL,
    TI_VAL_DATETIME,
    TI_VAL_NAME,
    TI_VAL_STR,
    TI_VAL_BYTES,       /* MP,STR and BIN all use RAW as underlying type */
    TI_VAL_REGEX,
    TI_VAL_THING,       /* instance or object */
    TI_VAL_WRAP,
    TI_VAL_ROOM,
    TI_VAL_TASK,
    TI_VAL_ARR,         /* array, list or tuple */
    TI_VAL_SET,         /* set of things */
    TI_VAL_ERROR,
    TI_VAL_MEMBER,      /* enum member */
    TI_VAL_MPDATA,      /* msgpack data */
    TI_VAL_CLOSURE,
    TI_VAL_ANO,         /* anonymous wrap-only type */
    TI_VAL_WANO,        /* wrapped with anonymous type */
    /* future, module and template are never stored */
    TI_VAL_FUTURE,      /* future */
    TI_VAL_MODULE,      /* module */
    TI_VAL_TEMPLATE,    /* template to generate TI_VAL_STR
                           note that a template is never stored like a value,
                           rather it may build from either a query or a stored
                           closure; therefore template does not need to be
                           handled like all other value type. */
} ti_val_enum;

enum
{
    TI_VFLAG_LOCK            =1<<7,      /* value in use;
                                            used to prevent illegal changes */
};

typedef enum
{
    /*
     * Reserved (may be implemented in the future):
     *   All specials are in the binary 0010xxxx range.
     *   + positive big type
     *   - negative big type
     */
    TI_KIND_C_INSTANCE  ='.',
    TI_KIND_C_OBJECT    =',',
    TI_KIND_C_SET       ='$',
    TI_KIND_C_ERROR     ='!',
    TI_KIND_C_WRAP      ='&',
    TI_KIND_C_MEMBER    ='%',
    TI_KIND_C_DATETIME  ='\'',
    TI_KIND_C_TIMEVAL   ='"',
    TI_KIND_C_WANO      =' ',
    /* Obsolete, but still required for backwards compatibility */
    TI_KIND_C_THING_OBSOLETE_       ='#',
    TI_KIND_C_CLOSURE_OBSOLETE_     ='/',
    TI_KIND_C_REGEX_OBSOLETE_       ='*',
} ti_val_kind;

typedef struct ti_val_s ti_val_t;

#include <inttypes.h>

struct ti_val_s
{
    /*
     * Both `ref` and `tp` MUST be implemented by all value type.
     *
     * Flags MUST be implemented when a type can be locked and in this case
     * the flag `TI_VFLAG_LOCK` MUST handle this lock.
     */
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    int:16;
};

#endif /* TI_VAL_T_H_ */
