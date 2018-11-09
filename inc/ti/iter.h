/*
 * ti/iter.h
 */
#ifndef TI_ITER_H_
#define TI_ITER_H_

#include <ti/prop.h>
#include <ti/name.h>

typedef ti_prop_t ti_iter_t[2];

ti_iter_t * ti_iter_create(void);
void ti_iter_destroy(ti_iter_t * iter);
static inline _Bool ti_iter_in_use_name(ti_iter_t * iter, ti_name_t * name);
static inline ti_val_t * ti_iter_get_val(ti_iter_t * iter, ti_name_t * name);

static inline _Bool ti_iter_in_use_name(ti_iter_t * iter, ti_name_t * name)
{
    return iter && (iter[0]->name == name || iter[1]->name == name);
}

static inline ti_val_t * ti_iter_get_val(ti_iter_t * iter, ti_name_t * name)
{
    return !iter ? NULL : (
            iter[0]->name == name ? &iter[0]->val : (
                    iter[1]->name == name ? &iter[1]->val : NULL));
}

#endif  /* TI_ITER_H_ */
