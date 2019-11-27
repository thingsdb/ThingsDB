/*
 * ti/qbind.h
 */
#ifndef TI_QBIND_H_
#define TI_QBIND_H_

enum
{
    /* first three flags are exclusive, only one may be set */
    TI_QBIND_FLAG_NODE          =1<<0,
    TI_QBIND_FLAG_THINGSDB      =1<<1,
    TI_QBIND_FLAG_COLLECTION    =1<<2,

    TI_QBIND_FLAG_EVENT         =1<<3,
    TI_QBIND_FLAG_WSE           =1<<4,
    TI_QBIND_FLAG_AS_PROCEDURE  =1<<5,

    TI_QBIND_FLAG_ON_VAR        =1<<6,
    TI_QBIND_FLAG_API           =1<<7,
};

typedef struct ti_qbind_s ti_qbind_t;

#include <inttypes.h>
#include <cleri/cleri.h>

void ti_qbind_probe(ti_qbind_t * qbind, cleri_node_t * nd);

struct ti_qbind_s
{
    uint32_t val_cache_n;       /* count while investigate */
    uint16_t pkg_id;            /* package id to return the query to */
    uint8_t flags;              /* query flags */
    uint8_t deep;               /* fetch level */
};


#endif /* TI_QBIND_H_ */
