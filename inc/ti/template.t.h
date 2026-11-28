/*
 * ti/template.h
 */
#ifndef TI_TEMPLATE_T_H_
#define TI_TEMPLATE_T_H_

typedef struct ti_template_s ti_template_t;

enum
{
    TI_TEMPLATE_FLAG_WSE    =1<<2,  /* stored template with side effects;
                                        when template may make changes they
                                        require a change. Must be checked
                                        for stored templates since only then
                                        is is possible no change is created
                                        */
    TI_TEMPLATE_FLAG_LOCK   =1<<3,
};

#include <cleri/cleri.h>
#include <ti/ncache.h>

struct ti_template_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    int:16;
    cleri_node_t *node;
    ti_ncache_t *ncache;
};

#endif  /* TI_TEMPLATE_T_H_ */
