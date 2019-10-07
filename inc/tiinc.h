/*
 * tiinc.h
 */
#ifndef TIINC_H_
#define TIINC_H_

#define TI_URL "https://thingsdb.github.io"

#define TI_DEFAULT_CLIENT_PORT 9200
#define TI_DEFAULT_NODE_PORT 9220

/* HTTP status port binding, 0=disabled, 1-65535=enabled */
#define TI_DEFAULT_HTTP_STATUS_PORT 0

/* Incremental events are stored until this threshold is reached */
#define TI_DEFAULT_THRESHOLD_FULL_STORAGE 1000UL

#define TI_COLLECTION_ID "`collection:%"PRIu64"`"
#define TI_EVENT_ID "`event:%"PRIu64"`"
#define TI_NODE_ID "`node:%"PRIu32"`"
#define TI_THING_ID "`#%"PRIu64"`"
#define TI_USER_ID "`user:%"PRIu64"`"
#define TI_SYNTAX "syntax v%u"
#define TI_SAVE_PACK 60 + ti_.nodes->imap->n * 140, 3

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
#include <cleri/cleri.h>
typedef struct ti_ref_s { uint32_t ref; } ti_ref_t;

#define ti_grab(x) ((x) && ++(x)->ref ? (x) : NULL)
#define ti_incref(x) (++(x)->ref)
#define ti_decref(x) (--(x)->ref)  /* use only when x->ref > 1 */
#define ti_max(x__, y__) ((x__) >= (y__) ? (x__) : (y__));
#define ti_min(x__, y__) ((x__) <= (y__) ? (x__) : (y__));

/* SUSv2 guarantees that "Host names are limited to 255 bytes,
 * excluding terminating null byte" */
#define TI_MAX_HOSTNAME_SZ 256

static const int TI_CLERI_PARSE_FLAGS =
    CLERI_FLAG_EXPECTING_DISABLED|
    CLERI_FLAG_EXCLUDE_OPTIONAL|
    CLERI_FLAG_EXCLUDE_FM_CHOICE|
    CLERI_FLAG_EXCLUDE_RULE_THIS;

enum
{
    TI_FLAG_SIGNAL          =1<<0,
    TI_FLAG_STOP            =1<<1,
    TI_FLAG_INDEXING        =1<<2,
    TI_FLAG_LOCKED          =1<<3,
    TI_FLAG_NODES_CHANGED   =1<<4,
};

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
