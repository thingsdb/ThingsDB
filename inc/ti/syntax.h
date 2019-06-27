/*
 * ti/syntax.h
 */
#ifndef TI_SYNTAX_H_
#define TI_SYNTAX_H_

enum
{
    TI_SYNTAX_FLAG_EVENT        =1<<0,
    TI_SYNTAX_FLAG_NESTED       =1<<1,
    TI_SYNTAX_FLAG_NODE         =1<<2,
    TI_SYNTAX_FLAG_THINGSDB     =1<<3,
    TI_SYNTAX_FLAG_COLLECTION   =1<<4,
};

typedef struct ti_syntax_s ti_syntax_t;

#include <inttypes.h>
#include <cleri/cleri.h>


void ti_syntax_init(ti_syntax_t * syntax);
void ti_syntax_investigate(ti_syntax_t * syntax, cleri_node_t * nd);

struct ti_syntax_s
{
    uint32_t nd_val_cache_n;    /* count while investigate */
    uint16_t pkg_id;            /* package id to return the query to */
    uint8_t flags;              /* query flags */
    uint8_t deep;               /* fetch level */
};

#endif /* TI_SYNTAX_H_ */
