/*
 * ti/qbind.t.h
 */
#ifndef TI_QBIND_T_H_
#define TI_QBIND_T_H_

#define TI_QBIND_FLAG_BIT = 3;

typedef enum
{
    /* First three flags are exclusive, only one may be set.
     * The order is important since the order is used in function binding */
    TI_QBIND_BIT_NODE,
    TI_QBIND_BIT_THINGSDB=1,      /* must be one (see qbind) */
    TI_QBIND_BIT_COLLECTION=2,    /* must be two (see qbind) */

    TI_QBIND_BIT_EVENT,
    TI_QBIND_BIT_WSE,
    TI_QBIND_BIT_AS_PROCEDURE,

    TI_QBIND_BIT_ON_VAR,
    TI_QBIND_BIT_API,
} ti_qbind_bit_t;

typedef enum
{
    /* first three flags are exclusive, only one may be set */
    TI_QBIND_FLAG_NODE          =1<<TI_QBIND_BIT_NODE,
    TI_QBIND_FLAG_THINGSDB      =1<<TI_QBIND_BIT_THINGSDB,
    TI_QBIND_FLAG_COLLECTION    =1<<TI_QBIND_BIT_COLLECTION,

    TI_QBIND_FLAG_EVENT         =1<<TI_QBIND_BIT_EVENT,
    TI_QBIND_FLAG_WSE           =1<<TI_QBIND_BIT_WSE,
    TI_QBIND_FLAG_AS_PROCEDURE  =1<<TI_QBIND_BIT_AS_PROCEDURE,

    TI_QBIND_FLAG_ON_VAR        =1<<TI_QBIND_BIT_ON_VAR,
    TI_QBIND_FLAG_API           =1<<TI_QBIND_BIT_API,
} ti_qbind_flag_t;

typedef struct ti_qbind_s ti_qbind_t;

#include <inttypes.h>

struct ti_qbind_s
{
    uint32_t val_cache_n;       /* count while investigate */
    uint16_t pkg_id;            /* package id to return the query to */
    uint8_t flags;              /* query flags (ti_qbind_flag_t) */
    uint8_t deep;               /* fetch level */
};


#endif /* TI_QBIND_T_H_ */
