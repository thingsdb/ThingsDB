/*
 * ti/syntax.h
 */
#ifndef TI_SYNTAX_H_
#define TI_SYNTAX_H_

enum
{
    TI_SYNTAX_FLAG_EVENT     =1<<0,
    TI_SYNTAX_FLAG_NESTED    =1<<1,
    TI_SYNTAX_FLAG_ALL       =1<<2,
    TI_SYNTAX_FLAG_NODE      =1<<3,
};

typedef struct ti_syntax_s ti_syntax_t;

void ti_syntax_init(ti_syntax_t * syntax);

struct ti_syntax_s
{
    uint32_t nd_val_cache_n;    /* count while investigate */
    uint16_t pkg_id;            /* pkg id to resturn to */
    uint8_t flags;              /* query flags */
    uint8_t deep;               /* fetch level */

};

#endif /* TI_SYNTAX_H_ */
