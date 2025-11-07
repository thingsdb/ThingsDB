/*
 * ti/wano.t.h
 */
#ifndef TI_WANO_T_H_
#define TI_WANO_T_H_

typedef struct ti_wano_s ti_wano_t;

#include <ti/ano.t.h>
#include <ti/thing.t.h>

struct ti_wano_s
{
    uint32_t ref;
    uint8_t tp;
    int:24;
    ti_ano_t * ano;         /* with reference */
    ti_thing_t * thing;     /* with reference */
};

#endif /* TI_WANO_T_H_ */
