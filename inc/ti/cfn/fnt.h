#include <ti/cfn/fn.h>

#define T_DOC_ TI_SEE_DOC("#t")

static int cq__f_t(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    const int nargs = langdef_nd_n_function_params(nd);
    ti_varr_t * varr = NULL;
    cleri_children_t * child = nd->children;    /* first in argument list */

    if (!nargs)
    {
        ex_set(e, EX_BAD_DATA,
                "function `t` requires at least 1 argument but 0 "
                "were given"T_DOC_);
        return e->nr;
    }

    assert (child);

    for (int arg = 1; child; child = child->next->next, ++arg)
    {
        ti_thing_t * thing;
        int64_t id;
        assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);

        if (ti_cq_scope(query, child->node, e))
            goto failed;

        if (!ti_val_is_int(query->rval))
        {
            ex_set(e, EX_BAD_DATA,
                "function `t` expects argument %d to be of "
                "type `"TI_VAL_INT_S"` but got type `%s` instead"T_DOC_,
                arg, ti_val_str(query->rval));
            goto failed;
        }

        id = ((ti_vint_t *) query->rval)->int_;

        thing = id > 0 ? ti_collection_thing_by_id(query->target, id) : NULL;

        if (!thing)
        {
            ex_set(e, EX_INDEX_ERROR,
                    "collection `%.*s` has no `thing` with id %"PRId64,
                    (int) query->target->name->n,
                    (char *) query->target->name->data,
                    id);
            goto failed;
        }

        ti_val_drop(query->rval);

        if (arg == 1)
        {
            if (!child->next)
            {
                assert (nargs == 1);
                ti_incref(thing);
                query->rval = (ti_val_t *) thing;
                if (ti_scope_push_thing(&query->scope, thing))
                    ex_set_alloc(e);

                return e->nr;  /* only one thing, no `varr` array */
            }

            varr = ti_varr_create(nargs);
            if (!varr)
            {
                ex_set_alloc(e);
                return e->nr;  /* save to return, `varr` is not created */
            }
        }

        query->rval = NULL;
        ti_incref(thing);
        VEC_push(varr->vec, thing);

        if (!child->next)
            break;
    }

    assert (varr);
    assert (varr->vec->n >= 1);

    query->rval = (ti_val_t *) varr;
    return 0;

failed:
    ti_val_drop((ti_val_t *) varr);
    return e->nr;
}
