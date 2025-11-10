/*
 * ti/ano.t.h
 */
#ifndef TI_ANO_T_H_
#define TI_ANO_T_H_

typedef struct ti_ano_s  ti_ano_t;

#include <ti/type.t.h>
#include <ti/val.t.h>

struct ti_ano_s
{
    uint32_t ref;
    uint8_t tp;
    int:24;
    ti_raw_t * spec_raw;  /* mpdata, in log use <anonymous> */
    ti_type_t * type;
};

#endif /* TI_ANO_T_H_ */
