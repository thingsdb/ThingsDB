/*
 * ti/template.h
 */
#ifndef TI_TEMPLATE_T_H_
#define TI_TEMPLATE_T_H_

typedef struct ti_template_s ti_template_t;

#include <cleri/cleri.h>

struct ti_template_s
{
    uint32_t ref;
    uint8_t tp;
    int:8;
    int:16;
    cleri_node_t *node;
};

#endif  /* TI_TEMPLATE_T_H_ */
