/*
 * langdef/nd.c
 */
#include <assert.h>
#include <langdef/langdef.h>
#include <langdef/nd.h>

_Bool langdef_nd_is_update_function(cleri_node_t * nd)
{
    cleri_t * function;

    if (!langdef_nd_is_function(nd))
        return false;

    function = nd                           /* sequence */
            ->children->node                /* choice */
            ->children->node->cl_obj;       /* keyword or identifier */

    switch (function->gid)
    {
    case CLERI_GID_F_CREATE:
    case CLERI_GID_F_DEL:
    case CLERI_GID_F_DROP:
    case CLERI_GID_F_GRANT:
    case CLERI_GID_F_PUSH:
    case CLERI_GID_F_RENAME:
    case CLERI_GID_F_REVOKE:
    case CLERI_GID_F_SET:
    case CLERI_GID_F_UNSET:
        return true;
    default:
        return false;
    }
}

/*
 * Returns a negative value when the argument is an iterator, or 0 when no
 * arguments are used, or the number of arguments.
 */
int langdef_nd_info_function_params(cleri_node_t * nd)
{
    assert (langdef_nd_is_function_params(nd));

    int n;
    cleri_children_t * child;

    if (nd->cl_obj->gid == CLERI_GID_ITERATOR)
        return -1;

    for (n = 0, child = nd->children; child; child = child->next->next)
    {
        ++n;
        if (!child->next)
            break;
    }
    return n;
}
