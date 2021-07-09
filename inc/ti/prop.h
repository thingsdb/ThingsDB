/*
 * ti/prop.h
 */
#ifndef TI_PROP_H_
#define TI_PROP_H_

#include <ti/prop.t.h>
#include <ti/name.t.h>
#include <ti/val.t.h>

ti_prop_t * ti_prop_create(ti_name_t * name, ti_val_t * val);
ti_prop_t * ti_prop_dup(ti_prop_t * prop);
void ti_prop_destroy(ti_prop_t * prop);
void ti_prop_unsafe_vdestroy(ti_prop_t * prop);

#endif /* TI_PROP_H_ */
