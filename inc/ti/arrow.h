/*
 * ti/arrow.h
 */
#ifndef TI_ARROW_H_
#define TI_ARROW_H_

enum
{
    TI_ARROW_FLAG            =1<<0,
    TI_ARROW_FLAG_WSE        =1<<1,     /* with side effects */
};

typedef struct ti_arrow_s ti_arrow_t;

#include <stdint.h>
#include <cleri/cleri.h>
#include <qpack.h>
#include <ti.h>

cleri_node_t * ti_arrow_from_strn(const char * str, size_t n);
void ti_arrow_destroy(cleri_node_t * arrow);
int ti_arrow_to_packer(cleri_node_t * arrow, qp_packer_t ** packer);
int ti_arrow_to_file(cleri_node_t * arrow, FILE * f);
uchar * ti_arrow_uchar(cleri_node_t * arrow, size_t * n);
static inline cleri_node_t * ti_arrow_scope_nd(cleri_node_t * arrow);
static inline _Bool ti_arrow_wse(cleri_node_t * arrow);
static inline cleri_node_t * ti_arrow_cp(cleri_node_t * arrow);

struct ti_arrow_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _pad8;
    uint16_t _pad16;

    cleri_node_t * node;
};

static inline _Bool ti_arrow_wse(cleri_node_t * arrow)
{
    return (
        arrow->str != arrow->data &&
        (((intptr_t) arrow->data) & TI_ARROW_FLAG_WSE)
    );
}

static inline cleri_node_t * ti_arrow_scope_nd(cleri_node_t * arrow)
{
    /*  arrow = Sequence(List(name, opt=False), '=>', scope)  */
    return arrow->children->next->next->node;
}

static inline cleri_node_t * ti_arrow_cp(cleri_node_t * arrow)
{
    return ti_arrow_from_strn(arrow->str, arrow->len);
}

#endif  /* TI_ARROW_H_ */
