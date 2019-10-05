#define _GNU_SOURCE
#include <ti/fn/fn.h>

typedef struct
{
    ti_query_t * query;
    ti_closure_t * closure;
    ex_t * e;
} closure_cmp_t;

int ti_closure_cmp(ti_val_t * va, ti_val_t * vb, closure_cmp_t * cc)
{
    assert (cc->closure->vars->n == 2);
    assert (cc->query->rval == NULL);

    int64_t i;
    ti_prop_t * pa, *pb;

    if (cc->e->nr)
        return 0;

    ti_incref(va);
    ti_incref(vb);

    pa = vec_get(cc->closure->vars, 0);
    pb = vec_get(cc->closure->vars, 1);

    ti_val_drop(pa->val);
    ti_val_drop(pb->val);

    pa->val = va;
    pb->val = vb;

    if (ti_closure_do_statement(cc->closure, cc->query, cc->e))
        return 0;

    if (!ti_val_is_int(cc->query->rval))
    {
        ex_set(cc->e, EX_TYPE_ERROR,
                "expecting a return value of type `"TI_VAL_INT_S"` "
                "but got type `%s` instead"DOC_SORT,
                ti_val_str(cc->query->rval));
        return 0;
    }

    i = ((ti_vint_t *) cc->query->rval)->int_;

    ti_val_drop(cc->query->rval);
    cc->query->rval = NULL;

    return i < INT_MIN ? INT_MIN : i > INT_MAX ? INT_MAX : i;
}

static int do__f_sort(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_varr_t * varr;
    ti_closure_t * closure;

    if (fn_not_chained("sort", query, e))
        return e->nr;

    if (    !ti_val_is_arr(query->rval) &&
            !ti_val_is_set(query->rval) &&
            !ti_val_is_thing(query->rval))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no function `sort`"DOC_SORT,
                ti_val_str(query->rval));
        return e->nr;
    }

    if (fn_nargs_max("sort", DOC_SORT, 1, nargs, e))
        return e->nr;

    varr = (ti_varr_t *) query->rval;
    query->rval = NULL;

    if (ti_varr_to_list(&varr))
    {
        ex_set_mem(e);
        goto fail0;
    }

    if (varr->vec->n <= 1)
        goto done;  /* nothing to sort */

    /* varr has only one reference and could be a copy, or if this was the
     * only reference it is just the old one */
    if (nargs == 0)
    {
        vec_sort_r(varr->vec, (vec_sort_r_cb) ti_opr_cmp, e);
        if (e->nr)
            goto fail0;
        goto done;
    }

    if (ti_do_statement(query, nd->children->node, e))
        goto fail0;

    if (!ti_val_is_closure(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `sort` expects argument 1 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead"DOC_SORT,
                ti_val_str(query->rval));
        goto fail0;
    }

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (closure->vars->n != 2)
    {
        ex_set(e, EX_NUM_ARGUMENTS,
            "function `sort` requires a closure which accepts 2 arguments"
            DOC_SORT);
        goto fail1;
    }

    if (    ti_closure_try_wse(closure, query, e) ||
            ti_closure_try_lock_and_use(closure, query, e))
        goto fail1;

    closure_cmp_t cc = {
            .query = query,
            .closure = closure,
            .e = e,
    };
    vec_sort_r(varr->vec, (vec_sort_r_cb) ti_closure_cmp, &cc);

    ti_closure_unlock_use(closure, query);

    if (e->nr)
        goto fail1;

    ti_val_drop((ti_val_t *) closure);

done:
    query->rval = (ti_val_t *) varr;
    return e->nr;

fail1:
    ti_val_drop((ti_val_t *) closure);

fail0:
    ti_val_drop((ti_val_t *) varr);
    return e->nr;
}
