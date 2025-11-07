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
int ti_do_bit_sl(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_bit_sr(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_bit_and(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_bit_xor(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_bit_or(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_compare_eq(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_compare_ne(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_compare_lt(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_compare_le(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_compare_gt(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_compare_ge(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_compare_and(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_compare_or(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_ternary(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_if_statement(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_return_nil(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_return_val(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_return_alt_deep(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_return_alt_flags(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_for_loop(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_continue(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_break(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_closure(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_block(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_prepare_for_loop(ti_query_t * query, cleri_node_t * vars_nd);
int ti_do_init(void);
void ti_do_drop(void);


static inline int ti_do_statement(
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    /* Calls ti_do_expression(..) or one of the operations(..) */
    return ((ti_do_cb) nd->children->data)(query, nd->children, e);
}

#endif /* TI_DO_H_ */
