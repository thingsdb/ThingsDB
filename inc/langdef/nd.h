/*
 * langdef/nd.h
 */
#ifndef LANGDEF_ND_H_
#define LANGDEF_ND_H_

#include <cleri/cleri.h>
#include <langdef/langdef.h>

_Bool langdef_nd_is_update_function(cleri_node_t * nd);
static inline _Bool langdef_nd_is_function(cleri_node_t * nd);



static inline _Bool langdef_nd_is_function(cleri_node_t * nd)
{
    return nd->cl_obj->gid == CLERI_GID_FUNCTION;
}

#endif  /* LANGDEF_ND_H_ */
