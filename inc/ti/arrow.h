/*
 * ti/arrow.h
 */
#ifndef TI_ARROW_H_
#define TI_ARROW_H_

enum
{
    TI_ARROW_FLAG_QBOUND        =1<<0,      /* bound to query string */
    TI_ARROW_FLAG_WSE           =1<<1,      /* with side effects */
};

typedef struct ti_arrow_s ti_arrow_t;

#include <stdint.h>
#include <cleri/cleri.h>
#include <qpack.h>
#include <ti.h>


ti_arrow_t * ti_arrow_from_node(cleri_node_t * node);
ti_arrow_t * ti_arrow_from_strn(const char * str, size_t n);
void ti_arrow_destroy(ti_arrow_t * arrow);
int ti_arrow_unbound(ti_arrow_t * arrow);
int ti_arrow_to_packer(ti_arrow_t * arrow, qp_packer_t ** packer);
int ti_arrow_to_file(ti_arrow_t * arrow, FILE * f);
uchar * ti_arrow_uchar(ti_arrow_t * arrow, size_t * n);
static inline cleri_node_t * ti_arrow_scope_nd(ti_arrow_t * arrow);
static inline _Bool ti_arrow_wse(ti_arrow_t * arrow);

struct ti_arrow_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    uint16_t _pad16;
    cleri_node_t * node;
};

static inline _Bool ti_arrow_wse(ti_arrow_t * arrow)
{
    return (
        arrow->node->str != arrow->node->data &&
        (((intptr_t) arrow->node->data) & TI_ARROW_FLAG_WSE)
    );
}

static inline cleri_node_t * ti_arrow_scope_nd(ti_arrow_t * arrow)
{
    /*  arrow = Sequence(List(name, opt=False), '=>', scope)  */
    return arrow->node->children->next->next->node;
}



#endif  /* TI_ARROW_H_ */
