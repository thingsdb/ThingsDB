/*
 * tiinc.h
 */
#ifndef TIINC_H_
#define TIINC_H_

#define TI_DEFAULT_CLIENT_PORT 9200
#define TI_DEFAULT_NODE_PORT 9220

/* HTTP status port binding, 0=disabled, 1-65535=enabled */
#define TI_DEFAULT_HTTP_STATUS_PORT 0

/* HTTP API port binding, 0=disabled, 1-65535=enabled */
#define TI_DEFAULT_HTTP_API_PORT 0

/* Incremental events are stored until this threshold is reached */
#define TI_DEFAULT_THRESHOLD_FULL_STORAGE 1000UL

/* Raise an error when packing a thing and this limit (20MiB) is reached */
#define TI_DEFAULT_RESULT_DATA_LIMIT 20971520UL

/* Use query cache for queries with a length equal or above this threshold */
#define TI_DEFAULT_THRESHOLD_QUERY_CACHE 160UL

/* Cached query expiration time in seconds */
#define TI_DEFAULT_CACHE_EXPIRATION_TIME 900UL

#define TI_COLLECTION_ID "`collection:%"PRIu64"`"
#define TI_EVENT_ID "`event:%"PRIu64"`"
#define TI_NODE_ID "`node:%"PRIu32"`"
#define TI_THING_ID "`#%"PRIu64"`"
#define TI_USER_ID "`user:%"PRIu64"`"
#define TI_QBIND "syntax v%u"

/* Max token expiration time */
#define TI_MAX_EXPIRATION_DOUBLE 4294967295.0
#define TI_MAX_EXPIRATION_LONG 4294967295L

/*
 * File name schema to check version info on created files.
 */
#define TI_FN_SCHEMA 0

/*
 * If a system has a WORDSIZE of 64 bits, we can take advantage of storing
 * some data in void pointers.
 */
#include <stdint.h>
#if UINTPTR_MAX == 0xffffffff
#define TI_IS64BIT 0
#elif UINTPTR_MAX == 0xffffffffffffffff
#define TI_IS64BIT 1
#else
#define TI_IS64BIT __WORDSIZE == 64
#endif

typedef unsigned char uchar;

typedef struct ti_s ti_t;
extern ti_t ti;

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
#include <cleri/cleri.h>
typedef struct ti_ref_s { uint32_t ref; } ti_ref_t;

#define ti_grab(x) ((x) && ++(x)->ref ? (x) : NULL)
#define ti_incref(x) (++(x)->ref)
#define ti_decref(x) (--(x)->ref)  /* use only when x->ref > 1 */
#define ti_max(x__, y__) ((x__) >= (y__) ? (x__) : (y__));
#define ti_min(x__, y__) ((x__) <= (y__) ? (x__) : (y__));

static const int TI_CLERI_PARSE_FLAGS =
    CLERI_FLAG_EXPECTING_DISABLED|
    CLERI_FLAG_EXCLUDE_OPTIONAL|
    CLERI_FLAG_EXCLUDE_FM_CHOICE|
    CLERI_FLAG_EXCLUDE_RULE_THIS;

enum
{
    TI_FLAG_SIGNAL          =1<<0,
    TI_FLAG_LOCKED          =1<<1,
    TI_FLAG_NODES_CHANGED   =1<<2,
};

typedef enum
{
    TI_STR_INFO
} ti_ext_tp;

static inline _Bool ti_is_reserved_key_strn(const char * str, size_t n)
{
    return n == 1 && (*str >> 4 == 2);
}

#endif  /* TIINC_H_ */
