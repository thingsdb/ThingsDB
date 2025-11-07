/*
 * ti/forloop.h
 */
#ifndef TI_FORLOOP_H_
#define TI_FORLOOP_H_

#include <cleri/cleri.h>
#include <ex.h>
#include <tiinc.h>
#include <ti/query.h>

int ti_forloop_no_iter(
        ti_query_t * query,
        cleri_node_t * vars_nd,
        cleri_node_t * code_nd,
        ex_t * e);

int ti_forloop_arr(
        ti_query_t * query,
        cleri_node_t * vars_nd,
        cleri_node_t * code_nd,
        ex_t * e);

int ti_forloop_set(
        ti_query_t * query,
        cleri_node_t * vars_nd,
        cleri_node_t * code_nd,
        ex_t * e);

int ti_forloop_thing(
        ti_query_t * query,
        cleri_node_t * vars_nd,
        cleri_node_t * code_nd,
        ex_t * e);

typedef int (*ti_forloop_t) (
        ti_query_t *,
        cleri_node_t *,
        cleri_node_t *,
        ex_t *);

static const ti_forloop_t ti_forloop_callbacks[22] = {
        ti_forloop_no_iter,         /* TI_VAL_NIL */
        ti_forloop_no_iter,         /* TI_VAL_INT */
        ti_forloop_no_iter,         /* TI_VAL_FLOAT */
        ti_forloop_no_iter,         /* TI_VAL_BOOL */
        ti_forloop_no_iter,         /* TI_VAL_DATETIME */
        ti_forloop_no_iter,         /* TI_VAL_NAME */
        ti_forloop_no_iter,         /* TI_VAL_STR */
        ti_forloop_no_iter,         /* TI_VAL_BYTES */
        ti_forloop_no_iter,         /* TI_VAL_REGEX */
        ti_forloop_thing,           /* TI_VAL_THING */
        ti_forloop_no_iter,         /* TI_VAL_WRAP */
        ti_forloop_no_iter,         /* TI_VAL_ROOM */
        ti_forloop_no_iter,         /* TI_VAL_TASK */
        ti_forloop_arr,             /* TI_VAL_ARR */
        ti_forloop_set,             /* TI_VAL_SET */
        ti_forloop_no_iter,         /* TI_VAL_ERROR */
        ti_forloop_no_iter,         /* TI_VAL_MEMBER */
        ti_forloop_no_iter,         /* TI_VAL_MPDATA */
        ti_forloop_no_iter,         /* TI_VAL_CLOSURE */
        ti_forloop_no_iter,         /* TI_VAL_FUTURE */
        ti_forloop_no_iter,         /* TI_VAL_MODULE */
        ti_forloop_no_iter,         /* TI_VAL_ANO */
        ti_forloop_no_iter,         /* TI_VAL_TEMPLATE */
};

static inline int ti_forloop_call(
        ti_query_t * query,
        cleri_node_t * vars_nd,
        cleri_node_t * code_nd,
        ex_t * e)
{
    return ti_forloop_callbacks[query->rval->tp](query, vars_nd, code_nd, e);
}

#endif  /* TI_VAL_INLINE_H_ */
