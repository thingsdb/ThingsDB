/*
 * ti/root.c
 */

#include <assert.h>
#include <ti/root.h>
#include <stdlib.h>

ti_root_t * ti_root_create(void)
{
    ti_root_t * root = malloc(sizeof(ti_root_t));
    if (!root)
        return NULL;

    root->tp = TI_ROOT_UNDEFINED;
    root->rval = NULL;

    return root;
}


void ti_root_destroy(ti_root_t * root)
{
    if (!root)
        return;
    ti_val_destroy(root->rval);
    free(root);
}

int ti_root_scope(ti_root_t * root, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_SCOPE);

    int nots = 0;
    cleri_node_t * node;
    cleri_children_t * nchild, * child = nd           /* sequence */
                    ->children;             /* first child, not */
    ti_scope_t * current_scope, * scope;

    for (nchild = child->node->children; nchild; nchild = nchild->next)
        ++nots;

    if (nots)
    {
        ex_set(e, EX_BAD_DATA, "cannot use `not` expressions at root");
        return e->nr;
    }

    child = child->next;
    node = child->node                      /* choice */
            ->children->node;               /* primitives, function,
                                               assignment, name, thing,
                                               array, compare, arrow */

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_ARRAY:
        ex_set(e, EX_BAD_DATA, "cannot create `array` at root");
        return e->nr;
    case CLERI_GID_ARROW:
        ex_set(e, EX_BAD_DATA, "cannot create `arrow-function` at root");
        return e->nr;
    case CLERI_GID_ASSIGNMENT:
        ex_set(e, EX_BAD_DATA, "cannot assign properties to root");
        return e->nr;
    case CLERI_GID_OPERATIONS:
        ex_set(e, EX_BAD_DATA, "cannot use operators at root");
        return e->nr;
    case CLERI_GID_FUNCTION:
        if (root__function(root, node, e))
            return e->nr;
        break;
    case CLERI_GID_NAME:
        if (root__scope_name(root, node, e))
            return e->nr;
        break;
    case CLERI_GID_PRIMITIVES:
        ex_set(e, EX_BAD_DATA, "cannot create `primitives` at root");
        return e->nr;
    case CLERI_GID_THING:
        ex_set(e, EX_BAD_DATA, "cannot create `thing` at root");
        return e->nr;
    default:
        assert (0);  /* all possible should be handled */
        return -1;
    }

    child = child->next;

    assert (child);

    node = child->node;
    assert (node->cl_obj->gid == CLERI_GID_INDEX);

    if (node->children)
    {
        ex_set(e, EX_BAD_DATA, "indexing is not supported at root");
        return e->nr;
    }

    child = child->next;
    if (!child)
        goto finish;

    node = child->node              /* optional */
            ->children->node;       /* chain */

    assert (node->cl_obj->gid == CLERI_GID_CHAIN);

    if (root__chain(root, node, e))
        goto finish;

finish:
    return e->nr;
}

static int root__chain(ti_root_t * root, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd-> == 0);
    return e->nr;
}

static int root__function(ti_root_t * root, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (langdef_nd_is_function(nd));

    cleri_node_t * fname, * params;

    fname = nd                      /* sequence */
            ->children->node        /* choice */
            ->children->node;       /* keyword or name node */

    params = nd                             /* sequence */
            ->children->next->next->node;   /* list of scope (arguments) */


    switch (fname->cl_obj->gid)
    {

    }
    ex_set(e, EX_INDEX_ERROR,
            "`%.*s` is undefined",
            fname->len,
            fname->str);
    return e->nr;
}

static int root__scope_name(ti_root_t * root, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_NAME);
    assert (ti_name_is_valid_strn(nd->str, nd->len));

    if (langdef_nd_match_str(nd, "databases"))
        root->tp = TI_ROOT_DATABASES;
    else if (langdef_nd_match_str(nd, "users"))
        root->tp = TI_ROOT_USERS;
    else if (langdef_nd_match_str(nd, "nodes"))
        root->tp = TI_ROOT_NODES;
    else if (langdef_nd_match_str(nd, "configuration"))
        root->tp = TI_ROOT_CONFIG;
    else
    {
        ex_set(e, EX_INDEX_ERROR,
                "property `%.*s` is undefined",
                (int) nd->len, nd->str);
        return e->nr;
    }

    return e->nr;
}
