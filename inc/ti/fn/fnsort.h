#define _GNU_SOURCE
#include <ti/fn/fn.h>

typedef struct
{
    ti_query_t * query;
    ti_closure_t * closure;
    ex_t * e;
    vec_sort_r_cb cb;
} closure_cmp_t;

static int ti_closure_cmp(ti_val_t * va, ti_val_t * vb, closure_cmp_t * cc)
{
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
                "but got type `%s` instead"DOC_LIST_SORT,
                ti_val_str(cc->query->rval));
        return 0;
    }

    i = VINT(cc->query->rval);

    ti_val_drop(cc->query->rval);
    cc->query->rval = NULL;

    return i < INT_MIN ? INT_MIN : i > INT_MAX ? INT_MAX : i;
}

static int ti_closure_pick(ti_val_t * va, ti_val_t * vb, closure_cmp_t * cc)
{
    int i;
    ti_prop_t * p;

    if (cc->e->nr)
        return 0;

    ti_incref(va);
    p = vec_get(cc->closure->vars, 0);
    ti_val_drop(p->val);
    p->val = va;

    if (ti_closure_do_statement(cc->closure, cc->query, cc->e))
        return 0;

    va = cc->query->rval;
    cc->query->rval = NULL;

    ti_incref(vb);
    p = vec_get(cc->closure->vars, 0);
    ti_val_drop(p->val);
    p->val = vb;

    if (ti_closure_do_statement(cc->closure, cc->query, cc->e))
    {
        ti_val_drop(va);
        return 0;
    }

    vb = cc->query->rval;
    cc->query->rval = NULL;

    i = cc->cb(va, vb, cc->e);

    ti_val_drop(va);
    ti_val_drop(vb);

    return i;
}

static int do__f_sort(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_varr_t * varr;
    ti_closure_t * closure;
    _Bool reverse = false;

    if (!ti_val_is_array(query->rval))
        return fn_call_try("sort", query, nd, e);

    if (fn_nargs_max("sort", DOC_LIST_SORT, 2, nargs, e))
        return e->nr;

    if (vec_is_sorting())
    {
        ex_set(e, EX_OPERATION_ERROR,
                "function `sort` cannot be used recursively"DOC_LIST_SORT,
                ti_val_str(query->rval));
        return e->nr;
    }

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
        vec_sort_r(varr->vec, (vec_sort_r_cb) ti_opr_compare, e);
        if (e->nr)
            goto fail0;
        goto done;
    }

    if (ti_do_statement(query, nd->children->node, e))
        goto fail0;

    if (nargs == 1 && ti_val_is_bool(query->rval))
    {
        reverse = VBOOL(query->rval);

        ti_val_drop(query->rval);
        query->rval = NULL;

        vec_sort_r(
                varr->vec,
                (vec_sort_r_cb) (
                        reverse
                        ? ti_opr_compare_desc
                        : ti_opr_compare
                ), e);

        if (e->nr)
            goto fail0;

        goto done;
    }

    if (!ti_val_is_closure(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `sort` expects argument 1 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead"
                DOC_LIST_SORT,
                ti_val_str(query->rval));
        goto fail0;
    }

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (closure->vars->n != 1 && closure->vars->n != 2)
    {
        ex_set(e, EX_NUM_ARGUMENTS,
                "function `sort` requires a closure which "
                "accepts 1 or 2 arguments"DOC_LIST_SORT);
        goto fail1;
    }

    if (nargs == 2)
    {
        if (ti_do_statement(query, nd->children->next->next->node, e))
            goto fail1;

        if (!ti_val_is_bool(query->rval))
        {
            ex_set(e, EX_TYPE_ERROR,
                    "function `sort` expects argument 2 to be "
                    "a `"TI_VAL_BOOL_S"` but got type `%s` instead"
                    DOC_LIST_SORT,
                    ti_val_str(query->rval));
            goto fail1;
        }

        reverse = VBOOL(query->rval);

        ti_val_drop(query->rval);
        query->rval = NULL;

        if (closure->vars->n == 2)
        {
            ex_set(e, EX_NUM_ARGUMENTS,
                "cannot specify an order with a closure which takes two "
                "arguments; in this case the order should be specified within "
                "the closure"DOC_LIST_SORT);
            goto fail1;
        }
    }

    if (    ti_closure_try_wse(closure, query, e) ||
            ti_closure_inc(closure, query, e))
        goto fail1;

    closure_cmp_t cc = {
            .query = query,
            .closure = closure,
            .e = e,
            .cb = (vec_sort_r_cb) (
                    reverse
                    ? ti_opr_compare_desc
                    : ti_opr_compare
            ),
    };
    vec_sort_r(
            varr->vec,
            (vec_sort_r_cb) (
                    closure->vars->n == 1
                    ? ti_closure_pick
                    : ti_closure_cmp
            ),
            &cc);

    ti_closure_dec(closure, query);

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
