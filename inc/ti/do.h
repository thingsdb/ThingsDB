/*
 * ti/do.h
 */
#ifndef TI_DO_H_
#define TI_DO_H_

#include <ti/ex.h>
#include <ti/query.h>
#include <cleri/cleri.h>
#include <langdef/langdef.h>
#include <inttypes.h>

int ti_do_scope(ti_query_t * query, cleri_node_t * nd, ex_t * e);
int ti_do_optscope(ti_query_t * query, cleri_node_t * nd, ex_t * e);
void ti__do_may_set_deep_(uint8_t * deep, cleri_children_t * child, ex_t * e);
static inline void ti_do_may_set_deep(
        uint8_t * deep,
        cleri_children_t * child,
        ex_t * e);

static inline void ti_do_may_set_deep(
        uint8_t * deep,
        cleri_children_t * child,
        ex_t * e)
{
    if (child && child->node->cl_obj->gid == CLERI_GID_DEEPHINT)
        ti__do_may_set_deep_(deep, child, e);
}

#endif /* TI_DO_H_ */
