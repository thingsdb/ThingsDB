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

/* flag LANGDEF_ND_FLAG_UNIQUE should not be used */
void langdef_nd_flag(cleri_node_t * nd, int flags)
{
    nd->data = (void *) ((intptr_t) flags);
    if (nd->data == nd->str)
    {
        assert ( !(flags & LANGDEF_ND_FLAG_UNIQUE));
        flags |= LANGDEF_ND_FLAG_UNIQUE;
        nd->data = (void *) ((intptr_t) flags);
    }
}
