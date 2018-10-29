/*
 * ti/res.c - query response
 */
#include <ti/res.h>
#include <stdlib.h>

ti_res_t * ti_res_create(ti_db_t * db)
{
    ti_res_t * res = malloc(sizeof(ti_res_t));
    if (!res)
        return NULL;
    res->db = db;  /* a borrowed reference since the query has one */
    res->collect = imap_create();
    if (ti_val_set(res->val, TI_VAL_THING, res->db->root) || !res->collect)
    {
        ti_res_destroy(res);
        return NULL;
    }
    return res;
}

void ti_res_destroy(ti_res_t * res)
{
    imap_destroy(res->collect, NULL);  /* TODO: maybe we do need a callback? */
    ti_val_destroy(res->val);
    free(res);
}

int res_scope(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert ((*nd)->cl_obj->gid == CLERI_GID_SCOPE);

    cleri_children_t * child = nd           /* sequence */
                    ->children;             /* first child, choice */

    cleri_node_t * node = child->node       /* choice */
            ->children->node;               /* primitives, function, identifier,
                                               thing, array, compare */

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_PRIMITIVES:
        /*
         * res->thing = Set to NULL
         * res->tp = NIL / BOOL / STRING / INT / FLOAT
         * res->value = union (actual value)
         */

    case CLERI_GID_FUNCTION:
        res__scope_function(res, nd, e);

        break;
    case CLERI_GID_IDENTIFIER:
        /*
         * res->thing = parent or null
         * res->tp = undefined if not found in parent
         * res->value = union (actual value)
         *
         */

        break;
    case CLERI_GID_THING:
        /*
         *
         */
        break;
    case CLERI_GID_ARRAY:
        /*
         *
         */
        break;
    case CLERI_GID_ARRAY:
        /*
         *
         */
        break;
    default:
        assert (0);  /* all possible should be handled */
        return -1;
    }

    child = child->next;
    if (!child)
        goto finish;

    node = child->node;
    if (node->cl_obj->gid == CLERI_GID_INDEX)
    {
        /* handle index */


        child = child->next;
        if (!child)
            goto finish;

        node = child->node;
    }

    /* handle follow-up */
    node = node                     /* optional */
            ->children->node        /* choice */
            ->children->node;       /* chain or assignment */

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_CHAIN:
        break;
    case CLERI_GID_ASSIGNMENT:
        break;
    default:
        assert (0);  /* all possible should be handled */
        return -1;
    }

finish:
    return 0;
}

static int res__index(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
}

static int res__scope_function(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert(langdef_nd_is_function(nd));

    cleri_node_t * fname, * params;

    fname = nd                      /* sequence */
            ->children->node        /* choice */
            ->children->node;       /* keyword or identifier node */

    params = nd                             /* sequence <func ( ... ) > */
            ->children->next->next->node    /* choice */
            ->children->node;               /* iterator or arguments */

    switch (fname->cl_obj->gid)
    {
    case CLERI_GID_F_ID:
        return res__f_id(res, nd, e);
    }

    ex_set(e, EX_INDEX_ERROR,
            "type `%s` has no function `%.*s`",
            ti_val_to_str(res->val),
            fname->len,
            fname->str);
    return e->nr;
}

static int res__f_id(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (langdef_nd_is_function_params(nd));

    int64_t thing_id;

    if (res->val->tp != TI_VAL_THING)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `id`",
                ti_val_to_str(res->val));
        return e->nr;
    }

    if (langdef_nd_has_function_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `id()` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    thing_id = res->val->via.thing->id;

    ti_val_clear(res->val);
    if (ti_val_set(res->val, TI_VAL_INT, &thing_id))
    {
        ex_set_alloc(e);
    }

    return e->nr;
}



