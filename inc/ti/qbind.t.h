/*
 * ti/qbind.t.h
 */
#ifndef TI_QBIND_T_H_
#define TI_QBIND_T_H_

typedef enum
{
    /* First three flags are exclusive, only one may be set.
     * The order is important since the order is used in function binding */
    TI_QBIND_BIT_NODE,
    TI_QBIND_BIT_THINGSDB=1,      /* must be one (see qbind, FN__FLAG_EV_T) */
    TI_QBIND_BIT_COLLECTION=2,    /* must be two (see qbind, FN__FLAG_EV_C) */

    TI_QBIND_BIT_WSE,
    TI_QBIND_BIT_ON_VAR,
} ti_qbind_bit_t;

typedef enum
{
    /* first three flags are exclusive, only one may be set */
    TI_QBIND_FLAG_NODE          =1<<TI_QBIND_BIT_NODE,
    TI_QBIND_FLAG_THINGSDB      =1<<TI_QBIND_BIT_THINGSDB,
    TI_QBIND_FLAG_COLLECTION    =1<<TI_QBIND_BIT_COLLECTION,

    TI_QBIND_FLAG_WSE           =1<<TI_QBIND_BIT_WSE,
    TI_QBIND_FLAG_ON_VAR        =1<<TI_QBIND_BIT_ON_VAR,
} ti_qbind_flag_t;

typedef struct ti_qbind_s ti_qbind_t;

#include <inttypes.h>

struct ti_qbind_s
{
    uint32_t immutable_n;   /* count while investigate */
    uint16_t fut_count;     /* running futures, at most TI_MAX_FUTURE_COUNT  */
    uint8_t flags;          /* query flags (ti_qbind_flag_t) */
    uint8_t deep;           /* fetch level */
};


#endif /* TI_QBIND_T_H_ */
