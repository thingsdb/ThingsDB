/*
 * ti/val.t.h
 */
#ifndef TI_VAL_T_H_
#define TI_VAL_T_H_

#define VAL__CAST_MAX 9223372036854775808.0

/*
 * enum cache is not a real value type but used for stored closure to pre-cache
 *
 */
#define TI_VAL_ENUM_CACHE 255

#define TI_VAL_NIL_S        "nil"
#define TI_VAL_INT_S        "int"
#define TI_VAL_FLOAT_S      "float"
#define TI_VAL_BOOL_S       "bool"
#define TI_VAL_INFO_S       "info"
#define TI_VAL_STR_S        "str"
#define TI_VAL_BYTES_S      "bytes"
#define TI_VAL_REGEX_S      "regex"
#define TI_VAL_THING_S      "thing"
#define TI_VAL_LIST_S       "list"
#define TI_VAL_TUPLE_S      "tuple"
#define TI_VAL_SET_S        "set"
#define TI_VAL_CLOSURE_S    "closure"
#define TI_VAL_ERROR_S      "error"
#define TI_VAL_DATETIME_S   "datetime"
#define TI_VAL_TIMEVAL_S    "timeval"

/* negative value is used for packing tasks */
#define TI_VAL_PACK_TASK -1
#define TI_VAL_PACK_FILE -2

#define TI_KIND_S_THING     "#"
#define TI_KIND_S_INSTANCE  "."
#define TI_KIND_S_CLOSURE   "/"
#define TI_KIND_S_REGEX     "*"
#define TI_KIND_S_SET       "$"
#define TI_KIND_S_ERROR     "!"
#define TI_KIND_S_WRAP      "&"
#define TI_KIND_S_MEMBER    "%"
#define TI_KIND_S_DATETIME  "'"
#define TI_KIND_S_TIMEVAL   "\""

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
    TI_VAL_ARR,         /* array, list or tuple */
    TI_VAL_SET,         /* set of things */
    TI_VAL_CLOSURE,
    TI_VAL_ERROR,
    TI_VAL_MEMBER,      /* enum member */
    TI_VAL_MP,          /* msgpack data */
    TI_VAL_TEMPLATE,    /* template to generate TI_VAL_STR
                           note that a template is never stored like a value,
                           rather it may build from either a query or a stored
                           closure; therefore template does not need to be
                           handled like all other value type. */
} ti_val_enum;

enum
{
    TI_VFLAG_THING_SWEEP     =1<<0,      /* marked for sweep; each thing is
                                            initially marked and while running
                                            garbage collection this mark is
                                            removed as long as the `thing` is
                                            attached to the collection. */
    TI_VFLAG_THING_NEW       =1<<1,      /* thing is new; new things require
                                            a full `dump` in a task while
                                            existing things only can contain
                                            the `id`.*/
    TI_VFLAG_CLOSURE_BTSCOPE =1<<2,      /* closure bound to query string
                                            within the thingsdb scope;
                                            when not stored, closures do not
                                            own the closure string but refer
                                            the full query string.*/
    TI_VFLAG_CLOSURE_BCSCOPE =1<<3,      /* closure bound to query string
                                            within a collection scope;
                                            when not stored, closures do not
                                            own the closure string but refer
                                            the full query string. */
    TI_VFLAG_CLOSURE_WSE     =1<<4,      /* stored closure with side effects;
                                            when closure make changes they
                                            require an event and thus must be
                                            wrapped by wse() so we can know
                                            an event is created.
                                            (only stored closures) */
    TI_VFLAG_LOCK            =1<<5,      /* thing or value in use;
                                            used to prevent illegal changes */
    TI_VFLAG_ARR_TUPLE       =1<<6,      /* array is immutable; nested, and
                                            only nested array's are tuples;
                                            once a tuple is direct assigned to
                                            a thing, it converts back to a
                                            mutable list. */
    TI_VFLAG_ARR_MHT         =1<<7,      /* array may-have-things; some code
                                            might skip arrays without this flag
                                            while searching for things; */
};

typedef enum
{
    /*
     * Reserved (may be implemented in the future):
     *   All specials are in the binary 0010xxxx range.
     *   + positive big type
     *   - negative big type
     */
    TI_KIND_C_THING     ='#',
    TI_KIND_C_INSTANCE  ='.',
    TI_KIND_C_CLOSURE   ='/',
    TI_KIND_C_REGEX     ='*',
    TI_KIND_C_SET       ='$',
    TI_KIND_C_ERROR     ='!',
    TI_KIND_C_WRAP      ='&',
    TI_KIND_C_MEMBER    ='%',
    TI_KIND_C_DATETIME  ='\'',
    TI_KIND_C_TIMEVAL   ='"',
} ti_val_kind;

typedef struct ti_val_s ti_val_t;

#include <inttypes.h>

struct ti_val_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    uint16_t _pad16;
};

#endif /* TI_VAL_T_H_ */
