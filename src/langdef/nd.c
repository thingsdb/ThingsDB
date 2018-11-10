/*
 * langdef/nd.c
 */
#include <assert.h>
#include <langdef/langdef.h>
#include <langdef/nd.h>

/*
 * Returns a negative value when the argument is an iterator, or 0 when no
 * arguments are used, or the number of arguments.
 */
int langdef_nd_info_function_params(cleri_node_t * nd)
{
    assert (langdef_nd_is_function_params(nd));

    int n;
    cleri_children_t * child;

    if (nd->cl_obj->gid == CLERI_GID_ARROW)
        return -1;

    for (n = 0, child = nd->children; child; child = child->next->next)
    {
        ++n;
        if (!child->next)
            break;
    }
    return n;
}
