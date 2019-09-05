/*
 * ti/opr.h
 */
#ifndef TI_OPR_H_
#define TI_OPR_H_

#include <cleri/cleri.h>
#include <ex.h>
#include <ti/val.h>

int ti_opr_a_to_b(ti_val_t * a, cleri_node_t * nd, ti_val_t ** b, ex_t * e);
_Bool ti__opr_eq_(ti_val_t * a, ti_val_t * b);
static inline _Bool ti_opr_eq(ti_val_t * a, ti_val_t * b);

static inline _Bool ti_opr_eq(ti_val_t * a, ti_val_t * b)
{
    return a == b || ti__opr_eq_(a, b);
}

#endif  /* TI_OPR_H_ */
