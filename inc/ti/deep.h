/*
 * ti/deep.h
 */
#ifndef TI_DEEP_H_
#define TI_DEEP_H_

#include <inttypes.h>
#include <ex.h>
#include <ti/val.t.h>

int ti_deep_from_val(ti_val_t * val, uint8_t * deep, ex_t * e);

#endif  /* TI_DEEP_H_ */
