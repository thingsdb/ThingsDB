#include <ti/fn/fn.h>

static int do__f_call(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    cleri_children_t * child = nd->children;    /* first in argument list */
    ti_closure_t * closure;
    vec_t * args;

    if (fn_not_chained("call", query, e))
        return e->nr;

    if (!ti_val_is_closure(query->rval))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no function `call`"DOC_CALL,
                ti_val_str(query->rval));
        return e->nr;
    }

    args = vec_new(nargs);
    if (!args)
    {
        ex_set_mem(e);
        return e->nr;
    }

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    while (child)
    {
        if (ti_do_statement(query, child->node, e))
            goto failed;

        VEC_push(args, query->rval);
        query->rval = NULL;

        if (!child->next)
            break;

        child = child->next->next;
    }

    (void) ti_closure_call(closure, query, args, e);

failed:
    vec_destroy(args, (vec_destroy_cb) ti_val_drop);
    ti_val_drop((ti_val_t *) closure);
    return e->nr;
}
