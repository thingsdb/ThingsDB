/*
 * langdef/nd.c
 */
#include <assert.h>
#include <langdef/langdef.h>
#include <langdef/nd.h>

/*
 * Returns the number of arguments.
 */
int langdef_nd_n_function_params(cleri_node_t * nd)
{
    int n;
    cleri_children_t * child;

    for (n = 0, child = nd->children; child; child = child->next->next)
    {
        ++n;
        if (!child->next)
            break;
    }
    return n;
}
