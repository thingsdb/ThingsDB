/*
 * ti/witem.t.h
 */
#ifndef TI_WITEM_T_H_
#define TI_WITEM_T_H_

typedef struct ti_witem_s  ti_witem_t;

#include <ti/raw.t.h>
#include <ti/val.t.h>

struct ti_witem_s
{
    ti_raw_t * key;     /* without reference */
    ti_val_t ** val;    /* without reference */
};

#endif /* TI_WITEM_T_H_ */
