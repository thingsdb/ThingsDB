/*
 * ti/do.h
 */
#ifndef TI_DO_H_
#define TI_DO_H_

#include <ti/query.h>
#include <cleri/cleri.h>
#include <ex.h>
#include <langdef/langdef.h>

typedef int (*ti_do_cb)(ti_query_t * query, cleri_node_t * nd, ex_t * e);

int ti_do_expression(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_operation(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_compare_and(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_compare_or(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_ternary(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_init(void);
void ti_do_drop(void);

static inline int ti_do_statement(
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    /* Calls ti_do_expression(..) or one of the operations(..) */
    return ((ti_do_cb) nd->children->node->data)(query, nd->children->node, e);
}

#endif /* TI_DO_H_ */
