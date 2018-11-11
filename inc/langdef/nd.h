/*
 * langdef/nd.h
 */
#ifndef LANGDEF_ND_H_
#define LANGDEF_ND_H_

#include <cleri/cleri.h>
#include <langdef/langdef.h>


static inline _Bool langdef_nd_is_function(cleri_node_t * nd);
static inline _Bool langdef_nd_has_function_params(cleri_node_t * nd);
int langdef_nd_n_function_params(cleri_node_t * nd);
static inline void langdef_nd_flag(cleri_node_t * nd, int flags);

static inline _Bool langdef_nd_is_function(cleri_node_t * nd)
{
    return nd->cl_obj->gid == CLERI_GID_FUNCTION;
}

static inline _Bool langdef_nd_has_function_params(cleri_node_t * nd)
{
    return nd->children;
}

static inline void langdef_nd_flag(cleri_node_t * nd, int flags)
{
    nd->data = (void *) ((intptr_t) flags);
}


#endif  /* LANGDEF_ND_H_ */
