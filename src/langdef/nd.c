/*
 * langdef/nd.c
 */
#include <assert.h>
#include <langdef/langdef.h>
#include <langdef/nd.h>

_Bool langdef_nd_is_update_function(cleri_node_t * nd)
{
    cleri_t * function;
    assert (nd->cl_obj);

    if (!langdef_nd_is_function(nd))
        return false;

    function = nd                           /* sequence */
            ->children->node                /* choice */
            ->children->node->cl_obj;       /* keyword or identifier */

    switch (function->gid)
    {
    case CLERI_GID_F_CREATE:
    case CLERI_GID_F_DELETE:
    case CLERI_GID_F_DROP:
    case CLERI_GID_F_GRANT:
    case CLERI_GID_F_PUSH:
    case CLERI_GID_F_RENAME:
    case CLERI_GID_F_REVOKE:
        return true;
    default:
        return false;
    }
}
