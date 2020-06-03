#include <ti/fn/fn.h>

typedef struct
{
    ex_t * e;
    ti_closure_t * closure;
    ti_query_t * query;
} every__walk_t;

static int every__walk_set(ti_thing_t * t, every__walk_t * w)
{
    _Bool every;
    if (ti_closure_vars_vset(w->closure, t) ||
        ti_closure_do_statement(w->closure, w->query, w->e))
        return -1;

    every = ti_val_as_bool(w->query->rval);
    ti_val_drop(w->query->rval);

    if (!every)
        return 1;  /* walking stops here */

    w->query->rval = NULL;
    return 0;
}


static int do__f_every(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const char * doc;
    const int nargs = langdef_nd_n_function_params(nd);
    _Bool every = true;
    ti_val_t * iterval;
    ti_closure_t * closure;

    doc = doc_every(query->rval);
    if (!doc)
        return fn_call_try("every", query, nd, e);

    if (fn_nargs("every", doc, 1, nargs, e))
        return e->nr;

    iterval = query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e) ||
        fn_arg_closure("every", doc, 1, query->rval, e))
        goto fail0;

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (    ti_closure_try_wse(closure, query, e) ||
            ti_closure_inc(closure, query, e))
        goto fail1;

    switch (iterval->tp)
    {
    case TI_VAL_ARR:
    {
        int64_t idx  = 0;
        vec_t * vec = VARR(iterval);

        for (vec_each(vec, ti_val_t, v), ++idx)
        {
            if (ti_closure_vars_val_idx(closure, v, idx))
            {
                ex_set_mem(e);
                goto fail2;
            }

            if (ti_closure_do_statement(closure, query, e))
                goto fail2;

            every = ti_val_as_bool(query->rval);
            ti_val_drop(query->rval);

            if (!every)
                break;

            query->rval = NULL;
        }
        break;
    }
    case TI_VAL_SET:
    {
        every__walk_t w = {
                .e = e,
                .closure = closure,
                .query = query,
        };
        switch (imap_walk(VSET(iterval), (imap_cb) every__walk_set, &w))
        {
        case -1:
            if (!e->nr)
                ex_set_mem(e);
            goto fail2;
        case 1:
            every = false;
        }
        break;
    }
    }

    query->rval = (ti_val_t *) ti_vbool_get(every);
fail2:
    ti_closure_dec(closure, query);
fail1:
    ti_val_drop((ti_val_t *) closure);
fail0:
    ti_val_drop(iterval);
    return e->nr;
}
