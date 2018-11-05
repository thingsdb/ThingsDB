/*
 * langdef/nd.h
 */
#ifndef LANGDEF_ND_H_
#define LANGDEF_ND_H_

#include <cleri/cleri.h>
#include <langdef/langdef.h>

_Bool langdef_nd_is_update_function(cleri_node_t * nd);
static inline _Bool langdef_nd_is_function(cleri_node_t * nd);
static inline _Bool langdef_nd_is_function_params(cleri_node_t * nd);
static inline _Bool langdef_nd_has_function_params(cleri_node_t * nd);
int langdef_nd_info_function_params(cleri_node_t * nd);


static inline _Bool langdef_nd_is_function(cleri_node_t * nd)
{
    return nd->cl_obj->gid == CLERI_GID_FUNCTION;
}

static inline _Bool langdef_nd_is_function_params(cleri_node_t * nd)
{
    return (
        nd->cl_obj->gid == CLERI_GID_ARGUMENTS ||
        nd->cl_obj->gid == CLERI_GID_ITERATOR
    );
}

static inline _Bool langdef_nd_has_function_params(cleri_node_t * nd)
{
    return (
        nd->cl_obj->gid == CLERI_GID_ITERATOR ||
        nd->children         /* List <scope> */
    );
}



#endif  /* LANGDEF_ND_H_ */
