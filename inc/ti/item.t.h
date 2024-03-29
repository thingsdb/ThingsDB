/*
 * ti/item.t.h
 */
#ifndef TI_ITEM_T_H_
#define TI_ITEM_T_H_

typedef struct ti_item_s  ti_item_t;

#include <ti/raw.t.h>
#include <ti/val.t.h>

struct ti_item_s
{
    ti_raw_t * key;     /* with reference, valid names must always be of type
                           ti_name_t, but casted to ti_raw_t  */
    ti_val_t * val;     /* with reference */
};

#endif /* TI_ITEM_T_H_ */
