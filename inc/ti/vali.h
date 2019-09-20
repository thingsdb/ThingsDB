/*
 * ti/vali.h
 */
#ifndef TI_VALI_H_
#define TI_VALI_H_

#include <ti/thing.h>
#include <ti/val.h>

static inline _Bool ti_val_is_object(ti_val_t * val)
{
    return val->tp == TI_VAL_THING && ti_thing_is_object((ti_thing_t *) val);
}

#endif  /* TI_VALI_H_ */
