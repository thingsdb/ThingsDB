/*
 * ti/forloop.h
 */
#ifndef TI_FORLOOP_H_
#define TI_FORLOOP_H_

#include <cleri/cleri.h>
#include <ex.h>
#include <tiinc.h>
#include <ti/query.h>

int ti_forloop_arr(
        ti_query_t * query,
        cleri_node_t * vars_nd,
        cleri_node_t * code_nd,
        ex_t * e);

static inline int ti_forloop_no_iter(
        ti_query_t * query,
        cleri_node_t * UNUSED(vars_nd),
        cleri_node_t * UNUSED(code_nd),
        ex_t * e)
{
    ex_set(e, EX_TYPE_ERROR,
            "type `%s` is not iterable", ti_val_str(query->rval));
    return e->nr;
}

typedef int (*ti_forloop_t) (
        ti_query_t *,
        cleri_node_t *,
        cleri_node_t *,
        ex_t *);


static ti_forloop_t ti_forloop_callbacks[21] = {
        ti_forloop_no_iter,         /* TI_VAL_NIL */
        ti_forloop_no_iter,         /* TI_VAL_INT */
        ti_forloop_no_iter,         /* TI_VAL_FLOAT */
        ti_forloop_no_iter,         /* TI_VAL_BOOL */
        ti_forloop_no_iter,         /* TI_VAL_DATETIME */
        ti_forloop_no_iter,         /* TI_VAL_NAME */
        ti_forloop_no_iter,         /* TI_VAL_STR */
        ti_forloop_no_iter,         /* TI_VAL_BYTES */
        ti_forloop_no_iter,         /* TI_VAL_REGEX */
        ti_forloop_no_iter,         /* TI_VAL_THING */
        ti_forloop_no_iter,         /* TI_VAL_WRAP */
        ti_forloop_no_iter,         /* TI_VAL_ROOM */
        ti_forloop_no_iter,         /* TI_VAL_TASK */
        ti_forloop_arr,             /* TI_VAL_ARR */
        ti_forloop_no_iter,         /* TI_VAL_SET */
        ti_forloop_no_iter,         /* TI_VAL_ERROR */
        ti_forloop_no_iter,         /* TI_VAL_MEMBER */
        ti_forloop_no_iter,         /* TI_VAL_MPDATA */
        ti_forloop_no_iter,         /* TI_VAL_CLOSURE */
        ti_forloop_no_iter,         /* TI_VAL_FUTURE */
        ti_forloop_no_iter,         /* TI_VAL_TEMPLATE */
};

#define ti_forloop_call(__q, __vars, __code, __e) \
    ti_forloop_callbacks[(__q)->rval->tp]((__q), (__vars), (__code),  (__e))

#endif  /* TI_VAL_INLINE_H_ */
