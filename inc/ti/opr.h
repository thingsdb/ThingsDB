/*
 * ti/opr.h
 */
#ifndef TI_OPR_H_
#define TI_OPR_H_

#include <cleri/cleri.h>
#include <ti/val.h>
#include <ti/ex.h>

int ti_opr_a_to_b(ti_val_t * a, cleri_node_t * nd, ti_val_t ** b, ex_t * e);
_Bool ti_opr_eq(ti_val_t * a, ti_val_t * b);

#endif  /* TI_OPR_H_ */
