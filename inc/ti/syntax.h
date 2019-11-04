/*
 * ti/syntax.h
 */
#ifndef TI_SYNTAX_H_
#define TI_SYNTAX_H_

enum
{
    /* first three flags are exclusive, only one may be set */
    TI_SYNTAX_FLAG_NODE         =1<<0,
    TI_SYNTAX_FLAG_THINGSDB     =1<<1,
    TI_SYNTAX_FLAG_COLLECTION   =1<<2,

    TI_SYNTAX_FLAG_EVENT        =1<<3,
    TI_SYNTAX_FLAG_WSE          =1<<4,
    TI_SYNTAX_FLAG_AS_PROCEDURE =1<<5,

    TI_SYNTAX_FLAG_ON_VAR       =1<<6,
};

typedef struct ti_syntax_s ti_syntax_t;

#include <inttypes.h>
#include <cleri/cleri.h>

void ti_syntax_probe(ti_syntax_t * syntax, cleri_node_t * nd);

struct ti_syntax_s
{
    uint32_t val_cache_n;       /* count while investigate */
    uint16_t pkg_id;            /* package id to return the query to */
    uint8_t flags;              /* query flags */
    uint8_t deep;               /* fetch level */
};


#endif /* TI_SYNTAX_H_ */
