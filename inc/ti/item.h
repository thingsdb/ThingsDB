/*
 * ti/item.h
 */
#ifndef TI_ITEM_H_
#define TI_ITEM_H_

#include <ti/item.t.h>
#include <ti/raw.t.h>
#include <ti/val.t.h>

ti_item_t * ti_item_create(ti_raw_t * raw, ti_val_t * val);
ti_item_t * ti_item_dup(ti_item_t * item);
void ti_item_unassign_destroy(ti_item_t * item);
void ti_item_unsafe_vdestroy(ti_item_t * item);

#endif /* TI_ITEM_H_ */
