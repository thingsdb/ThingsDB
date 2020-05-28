/*
 * ti/preopr.h
 */
#ifndef TI_PREOPR_H_
#define TI_PREOPR_H_

#include <ex.h>
#include <ti/val.h>

int ti_preopr_bind(const char * s, size_t n);
int ti_preopr_calc(int preopr, ti_val_t ** val, ex_t * e);


#endif  /* TI_PREOPR_H_ */
