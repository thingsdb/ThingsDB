/*
 * ti/varr.h
 */
#ifndef TI_VARR_H_
#define TI_VARR_H_

#include <ex.h>
#include <stdint.h>
#include <ti/val.t.h>
#include <ti/varr.t.h>
#include <util/vec.h>

ti_varr_t * ti_varr_create(size_t sz);
ti_varr_t * ti_varr_from_vec(vec_t * vec);
ti_varr_t * ti_tuple_from_vec(vec_t * vec);
ti_varr_t * ti_varr_from_slice(
        ti_varr_t * source,
        ssize_t start,
        ssize_t stop,
        ssize_t step);
void ti_varr_destroy(ti_varr_t * varr);
int ti_varr_to_list(ti_varr_t ** varr);
int ti_varr_copy(ti_varr_t ** varr, uint8_t deep);
int ti_varr_dup(ti_varr_t ** varr, uint8_t deep);
int ti_varr_val_prepare(ti_varr_t * to, void ** v, ex_t * e);
int ti_varr_set(ti_varr_t * to, void ** v, size_t idx, ex_t * e);
_Bool ti__varr_eq(ti_varr_t * varra, ti_varr_t * varrb);
_Bool ti_varr_has_val(ti_varr_t * varr, ti_val_t * val);


#endif  /* TI_VARR_H_ */

