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


#include <stdint.h>
#include <cleri/cleri.h>
#include <qpack.h>
#include <ti.h>

int ti_arrow_to_packer(cleri_node_t * arrow, qp_packer_t * packer);
int ti_arrow_to_file(cleri_node_t * arrow, FILE * f);
uchar * ti_arrow_uchar(cleri_node_t * arrow, size_t * n);
static inline _Bool ti_arrow_wse(cleri_node_t * arrow);

static inline _Bool ti_arrow_wse(cleri_node_t * arrow)
{
    return (
        arrow->str != arrow->data &&
        (((intptr_t) arrow->data) & TI_ARROW_FLAG_WSE)
    );
}

#endif  /* TI_ARROW_H_ */
