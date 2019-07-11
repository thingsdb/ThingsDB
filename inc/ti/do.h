/*
 * ti/do.h
 */
#ifndef TI_DO_H_
#define TI_DO_H_

#include <ti/ex.h>
#include <ti/query.h>
#include <cleri/cleri.h>

int ti_do_scope(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_optscope(ti_query_t * query, cleri_node_t * nd, ex_t * e);

#endif /* TI_DO_H_ */
