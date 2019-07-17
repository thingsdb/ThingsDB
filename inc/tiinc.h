/*
 * tiinc.h
 */
#ifndef TIINC_H_
#define TIINC_H_

#define TI_URL "https://thingsdb.github.io"
#define TI_DOC(__fn) TI_URL"/ThingsDocs/"__fn
#define TI_SEE_DOC(__fn) "; see "TI_DOC(__fn)

#define TI_DEFAULT_CLIENT_PORT 9200
#define TI_DEFAULT_NODE_PORT 9220

/* HTTP status port binding, 0=disabled, 1-65535=enabled */
#define TI_DEFAULT_HTTP_STATUS_PORT 0

/* Incremental events are stored until this threshold is reached */
#define TI_DEFAULT_THRESHOLD_FULL_STORAGE 1000UL

#define TI_COLLECTION_ID "`collection:%"PRIu64"`"
#define TI_EVENT_ID "`event:%"PRIu64"`"
#define TI_NODE_ID "`node:%u`"
#define TI_THING_ID "`#%"PRIu64"`"
#define TI_USER_ID "`user:%"PRIu64"`"
#define TI_SYNTAX "syntax v%u"
#define TI_SCOPE_NODE 1
#define TI_SCOPE_THINGSDB 0

/* Max token expiration time */
#define TI_MAX_EXPIRATION_DOUDLE 4294967295.0
#define TI_MAX_EXPIRATION_LONG 4294967295L


/*
 * File name schema to check version info on created files.
 */
#define TI_FN_SCHEMA 0

/*
 * If a system has a WORDSIZE of 64 bits, we can take advantage of storing
 * some data in void pointers.
 */
#define TI_USE_VOID_POINTER __WORDSIZE == 64

typedef unsigned char uchar;

typedef struct ti_s ti_t;
extern ti_t ti_;

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

#define TI_CLOCK_MONOTONIC CLOCK_MONOTONIC_RAW

#include <inttypes.h>
typedef struct ti_ref_s { uint32_t ref; } ti_ref_t;

#define ti_grab(x) ((x) && ++(x)->ref ? (x) : NULL)
#define ti_incref(x) (++(x)->ref)
#define ti_decref(x) (--(x)->ref)  /* use only when x->ref > 1 */

/* SUSv2 guarantees that "Host names are limited to 255 bytes,
 * excluding terminating null byte" */
#define TI_MAX_HOSTNAME_SZ 256

enum
{
    TI_FLAG_SIGNAL          =1<<0,
    TI_FLAG_STOP            =1<<1,
    TI_FLAG_INDEXING        =1<<2,
    TI_FLAG_LOCKED          =1<<3,
};

typedef enum
{
    /* unknown function */
    TI_FN_0,

    /* collection functions */
    TI_FN_ADD,
    TI_FN_ARRAY,
    TI_FN_ASSERT,
    TI_FN_BLOB,
    TI_FN_BOOL,
    TI_FN_CONTAINS,
    TI_FN_DEL,
    TI_FN_ENDSWITH,
    TI_FN_ERR,
    TI_FN_FILTER,
    TI_FN_FIND,
    TI_FN_FINDINDEX,
    TI_FN_FLOAT,
    TI_FN_HAS,
    TI_FN_HASPROP,
    TI_FN_ID,
    TI_FN_INDEXOF,
    TI_FN_INT,
    TI_FN_ISARRAY,
    TI_FN_ISASCII,
    TI_FN_ISBOOL,
    TI_FN_ISERROR,
    TI_FN_ISFLOAT,
    TI_FN_ISINF,
    TI_FN_ISINT,
    TI_FN_ISLIST,
    TI_FN_ISNAN,
    TI_FN_ISNIL,
    TI_FN_ISRAW,
    TI_FN_ISSET,
    TI_FN_ISTHING,
    TI_FN_ISTUPLE,
    TI_FN_ISSTR,        /* alias */
    TI_FN_ISUTF8,
    TI_FN_LEN,
    TI_FN_LOWER,
    TI_FN_MAP,
    TI_FN_NOW,
    TI_FN_POP,
    TI_FN_PUSH,
    TI_FN_RAISE,
    TI_FN_REFS,
    TI_FN_REMOVE,
    TI_FN_RENAME,
    TI_FN_SET,
    TI_FN_SPLICE,
    TI_FN_STARTSWITH,
    TI_FN_STR,
    TI_FN_T,
    TI_FN_TEST,
    TI_FN_TRY,
    TI_FN_TYPE,
    TI_FN_UPPER,

    /* both thingsdb and collection scopes */
    TI_FN_CALL,
    TI_FN_CALLE,
    TI_FN_DEL_PROCEDURE,
    TI_FN_NEW_PROCEDURE,
    TI_FN_PROCEDURES_INFO,
    TI_FN_PROCEDURE_DEF,
    TI_FN_PROCEDURE_DOC,
    TI_FN_PROCEDURE_INFO,

    /* thingsdb functions */
    TI_FN_COLLECTION_INFO,
    TI_FN_COLLECTIONS_INFO,
    TI_FN_DEL_COLLECTION,
    TI_FN_DEL_EXPIRED,
    TI_FN_DEL_TOKEN,
    TI_FN_DEL_USER,
    TI_FN_GRANT,
    TI_FN_NEW_COLLECTION,
    TI_FN_NEW_NODE,
    TI_FN_NEW_TOKEN,
    TI_FN_NEW_USER,
    TI_FN_POP_NODE,
    TI_FN_RENAME_COLLECTION,
    TI_FN_RENAME_USER,
    TI_FN_REPLACE_NODE,
    TI_FN_REVOKE,
    TI_FN_SET_PASSWORD,
    TI_FN_SET_QUOTA,
    TI_FN_USER_INFO,
    TI_FN_USERS_INFO,

    /* node functions */
    TI_FN_COUNTERS,
    TI_FN_NODE_INFO,
    TI_FN_NODES_INFO,
    TI_FN_RESET_COUNTERS,
    TI_FN_SET_LOG_LEVEL,
    TI_FN_SHUTDOWN,

    /* explicit error functions */
    TI_FN_OVERFLOW_ERR,
    TI_FN_ZERO_DIV_ERR,
    TI_FN_MAX_QUOTA_ERR,
    TI_FN_AUTH_ERR,
    TI_FN_FORBIDDEN_ERR,
    TI_FN_INDEX_ERR,
    TI_FN_BAD_DATA_ERR,
    TI_FN_SYNTAX_ERR,
    TI_FN_NODE_ERR,
    TI_FN_ASSERT_ERR,
} ti_fn_enum_t;

typedef enum
{
    TI_a = 'a',
    TI_b,
    TI_c,
    TI_d,
    TI_e,
    TI_f,
    TI_g,
    TI_h,
    TI_i,
    TI_j,
    TI_k,
    TI_l,
    TI_m,
    TI_n,
    TI_o,
    TI_p,
    TI_q,
    TI_r,
    TI_s,
    TI_t,
    TI_u,
    TI_v,
    TI_w,
    TI_x,
    TI_y,
    TI_z,
} ti_alpha_lower_t;

#endif  /* TIINC_H_ */
