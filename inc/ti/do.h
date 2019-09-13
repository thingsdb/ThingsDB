/*
 * ti/do.h
 */
#ifndef TI_DO_H_
#define TI_DO_H_

#include <ti/query.h>
#include <cleri/cleri.h>
#include <ex.h>

int ti_do_statement(ti_query_t * query, cleri_node_t * nd, ex_t * e);

#endif /* TI_DO_H_ */
