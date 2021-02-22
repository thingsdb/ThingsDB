#include <ti/fn/fn.h>

static int do__f_future(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    cleri_children_t * child = nd->children;
    ti_future_t * future;
    ti_module_t * module;
    uint8_t deep;
    size_t num_args = nargs;

    if (fn_nargs_min("future", DOC_FUTURE, 1, nargs, e))
        return e->nr;

    if (ti.futures_count >= TI_MAX_FUTURE_COUNT)
    {
        ex_set(e, EX_MAX_QUOTA,
                "maximum number of active futures (%u) is reached",
                TI_MAX_FUTURE_COUNT);
        return e->nr;
    }

    if (ti_do_statement(query, child->node, e))
        return e->nr;

    switch(query->rval->tp)
    {
    case TI_VAL_THING:
    {
        ti_thing_t * thing = (ti_thing_t *) query->rval;
        ti_name_t * module_name = (ti_name_t *) ti_val_borrow_module_name();
        ti_name_t * deep_name = (ti_name_t *) ti_val_borrow_deep_name();
        ti_val_t * module_val = ti_thing_val_weak_by_name(thing, module_name);
        ti_val_t * deep_val = ti_thing_val_weak_by_name(thing, deep_name);

        if (!module_val)
        {
            ex_set(e, EX_LOOKUP_ERROR,
                    "missing `module` in future request"DOC_FUTURE);
            return e->nr;
        }

        if (!ti_val_is_str(module_val))
        {
            ex_set(e, EX_TYPE_ERROR,
                    "expecting `module` to be of type `"TI_VAL_STR_S"` "
                    "but got type `%s` instead"DOC_FUTURE,
                    ti_val_str(module_val));
            return e->nr;
        }

        module = ti_modules_by_raw((ti_raw_t *) module_val);
        if (!module)
            return ti_raw_err_not_found((ti_raw_t *) module_val, "module", e);

        if (module->scope_id && *module->scope_id != ti_query_scope_id(query))
        {
            ex_set(e, EX_FORBIDDEN,
                    "module `%s` is restricted to scope `%s`",
                    module->name->str,
                    ti_scope_name_from_id(*module->scope_id));
            return e->nr;
        }

        if (deep_val)
        {
            int64_t deepi;

            if (!ti_val_is_int(deep_val))
            {
                ex_set(e, EX_TYPE_ERROR,
                        "expecting `deep` to be of type `"TI_VAL_INT_S"` "
                        "but got type `%s` instead"DOC_FUTURE,
                        ti_val_str(module_val));
                return e->nr;
            }

            deepi = VINT(query->rval);

            if (deepi < 0 || deepi > MAX_DEEP_HINT)
            {
                ex_set(e, EX_VALUE_ERROR,
                        "expecting a `deep` value between 0 and %d "
                        "but got %"PRId64" instead",
                        MAX_DEEP_HINT, deepi);
                return e->nr;
            }

            deep = (uint8_t) deepi;
        }
        else
            deep = 1;
        break;
    }
    case TI_VAL_CLOSURE:
        if (ti_closure_unbound((ti_closure_t *) query->rval, e) ||
            ti_closure_inc_future((ti_closure_t *) query->rval, e))
            return e->nr;
        num_args = ((ti_closure_t *) query->rval)->vars->n;
        /* fall through */
    case TI_VAL_NIL:
        module = ti_async_get_module();
        deep = 0;
        break;
    default:
        ex_set(e, EX_TYPE_ERROR,
            "function `future` expects argument 1 to be of type "
            "`"TI_VAL_THING_S"`, `"TI_VAL_CLOSURE_S"` or `"TI_VAL_NIL_S"` "
            "but got type `%s` instead"DOC_FUTURE,
            ti_val_str(query->rval));
        return e->nr;
    }

    future = ti_future_create(query, module, num_args, deep);
    if (!future)
    {
        ex_set_mem(e);
        return e->nr;
    }
    if (ti_val_is_closure(query->rval))
    {
        for(vec_each(((ti_closure_t *) query->rval)->vars, ti_prop_t, prop))
        {
            ti_prop_t * p = ti_query_var_get(query, prop->name);
            if (!p)
            {
                ex_set(e, EX_LOOKUP_ERROR,
                        "variable `%s` is undefined",
                        prop->name->str);
                goto fail;
            }
            VEC_push(future->args, p->val);
            ti_incref(p->val);
        }
        future->then = (ti_closure_t *) query->rval;
        query->rval = NULL;
    }
    else
    {
        VEC_push(future->args, query->rval);
        query->rval = NULL;

        while ((child = child->next) && (child = child->next))
        {
            if (ti_do_statement(query, child->node, e))
                goto fail;

            VEC_push(future->args, query->rval);
            query->rval = NULL;
        }
    }

    if (ti_future_register(future))
    {
        ex_set_mem(e);
        goto fail;
    }

    query->rval = (ti_val_t *) future;
    return e->nr;

fail:
    ti_val_unsafe_drop((ti_val_t *) future);
    return e->nr;
}

