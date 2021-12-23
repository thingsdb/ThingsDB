/*
 * ti/forloop.c
 */
#include <ti/forloop.h>
#include <ti/nil.h>
#include <ti/val.inline.h>


int ti_forloop_arr(
        ti_query_t * query,
        cleri_node_t * vars_nd,
        cleri_node_t * code_nd,
        ex_t * e)
{
    const int nargs = (int) ((intptr_t) vars_nd->data);
    int lock_was_set = ti_val_ensure_lock(query->rval);
    ti_varr_t * varr = query->rval;
    int64_t idx = 0;

    if (varr->vec->n)
    {
        int n = nargs;
        cleri_children_t * child = vars_nd->children;
        do
        {

            name = name_nd->data
                    ? name_nd->data
                    : do__ensure_name_cache(query, name_nd);
            if (!name)
                goto alloc_err;

            /*
             * Check if the `prop` already is available in this scope on the
             * stack, and if * this is the case, then update the `prop` value with the
             * new value and return.
             */
            prop = do__prop_scope(query, name);
            if (prop)
            {
                ti_val_unsafe_gc_drop(prop->val);
                prop->val = query->rval;
                ti_incref(prop->val);
                return e->nr;
            }

            /*
             * Create a new `prop` and store the `prop` in this scope on the
             * stack. Only allocation errors might screw things up.
             */
            ti_incref(name);
            prop = ti_prop_create(name, query->rval);
            if (!prop)
                goto alloc_err;

            if (vec_push(&query->vars, prop))
                goto alloc_err_with_prop;


            if (!--n)
                break;
            child = child->next->next;
        }
        while (1);
    }
    for (vec_each(varr->vec, ti_val_t, v), ++idx)
    {
        switch(nargs)
        {
        default:
        case 2:
            prop = VEC_get(closure->vars, 1);
            ti_val_unsafe_drop(prop->val);
            prop->val = (ti_val_t *) ti_vint_create(i);
            if (!prop->val)
                return -1;
            /* fall through */
        case 1:
            prop = VEC_get(closure->vars, 0);
            ti_incref(v);
            ti_val_unsafe_drop(prop->val);
            prop->val = v;
            /* fall through */
        case 0:
            break;
        }


        query->rval = NULL;
        switch (ti_do_statement(query, code_nd, e))
        {
        case EX_SUCCESS:
            ti_val_unsafe_drop(query->rval);
            continue;
        case EX_CONTINUE:
            ti_val_drop(query->rval);  /* may be NULL */
            continue;
        case EX_BREAK:
            ti_val_drop(query->rval);  /* may be NULL */
            goto done;  /* success, but stop the loop */
        }
        goto fail0;  /* EX_RETURN must leave the value alone */
    }

done:
    query->rval = (ti_val_t *) ti_nil_get();

fail0:
    ti_val_unlock((ti_val_t *) varr, lock_was_set);
    ti_val_unsafe_drop((ti_val_t *) varr);
    return e->nr;
}
