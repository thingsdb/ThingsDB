#include <ti/fn/fn.h>

static ti_varr_t * future__args(
        ti_query_t * query,
        cleri_node_t * child,
        ex_t * e)
{
    ti_varr_t * varr;
    ti_closure_t * closure = (ti_closure_t *) query->rval;
    query->rval = NULL;
    if (ti_do_statement(query, child->next->next, e) ||
        fn_arg_array("future", DOC_FUTURE, 2, query->rval, e))
        goto fail;

    varr = (ti_varr_t *) query->rval;
    if (varr->vec->n != closure->vars->n)
    {
        ex_set(e, EX_NUM_ARGUMENTS,
                "number of future closure arguments must match the "
                "arguments in the provided list");
        goto fail;
    }
    query->rval = (ti_val_t *) closure;
    return varr;
fail:
    ti_val_unsafe_drop((ti_val_t *) closure);
    return NULL;
}

static int do__f_future(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    cleri_node_t * child = nd->children;
    ti_future_t * future;
    ti_module_t * module;
    ti_varr_t * varr = NULL;
    uint8_t deep = query->qbind.deep;
    size_t num_args = nargs;
    _Bool load = TI_MODULE_DEFAULT_LOAD;

    if (fn_nargs_min("future", DOC_FUTURE, 1, nargs, e))
        return e->nr;

    if (ti.futures_count >= TI_MAX_FUTURE_COUNT)
    {
        ex_set(e, EX_MAX_QUOTA,
                "maximum number of active futures (%u) is reached",
                TI_MAX_FUTURE_COUNT);
        return e->nr;
    }

    if (ti_do_statement(query, child, e))
        return e->nr;

    switch(query->rval->tp)
    {
    case TI_VAL_THING:
    {
        ti_thing_t * thing = (ti_thing_t *) query->rval;
        ti_name_t * module_name = (ti_name_t *) ti_val_borrow_module_name();
        ti_val_t * module_val = ti_thing_val_weak_by_name(thing, module_name);

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

        if (ti_module_read_args(module, thing, &load, &deep, e))
            return e->nr;

        if (ti_module_set_defaults(
                (ti_thing_t **) &query->rval,
                module->manifest.defaults))
        {
            ex_set_mem(e);
            return e->nr;
        }
        break;
    }
    case TI_VAL_CLOSURE:
        if (nargs == 2)
        {
            varr = future__args(query, child, e);
            if (!varr)
                return e->nr;
        }
        else if (nargs > 2)
        {
            ex_set(e, EX_NUM_ARGUMENTS,
                    "function `future` expects at most 2 arguments "
                    "when the first argument is "
                    "of type `"TI_VAL_CLOSURE_S"`"DOC_FUTURE);
            return e->nr;
        }
        if (ti_closure_unbound((ti_closure_t *) query->rval, e) ||
            ti_closure_inc_future((ti_closure_t *) query->rval, e))
            goto fail0;
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

    future = ti_future_create(query, module, num_args, deep, load);
    if (!future)
    {
        ex_set_mem(e);
        goto fail1;
    }

    if (ti_val_is_closure(query->rval))
    {
        /*
         * We are will register this future so arguments must be set. In case
         * of a closure this means we might have empty arguments thus may
         * require an empty list of arguments.
         */
        if (!future->args)
        {
            future->args = vec_new(0);
            if (!future->args)
            {
                ex_set_mem(e);
                goto fail2;
            }
        }
        else if (varr)
        {
            for (vec_each(varr->vec, ti_val_t, v))
            {
                VEC_push(future->args, v);
                ti_incref(v);
            }
            ti_val_unsafe_drop((ti_val_t *) varr);
            varr = NULL;
        }
        else for(vec_each(((ti_closure_t *) query->rval)->vars, ti_prop_t, prop))
        {
            ti_prop_t * p = ti_query_var_get(query, prop->name);
            if (!p)
            {
                ex_set(e, EX_LOOKUP_ERROR,
                        "variable `%s` is undefined",
                        prop->name->str);
                goto fail2;
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
            if (ti_do_statement(query, child, e))
                goto fail2;

            VEC_push(future->args, query->rval);
            query->rval = NULL;
        }
    }

    if (ti_future_register(future))
    {
        ex_set_mem(e);
        goto fail2;
    }

    query->rval = (ti_val_t *) future;
    return e->nr;

fail2:
    ti_val_unsafe_drop((ti_val_t *) future);
fail1:
    if (query->rval && ti_val_is_closure(query->rval))
        ti_closure_dec_future((ti_closure_t *) query->rval);
fail0:
    ti_val_drop((ti_val_t *) varr);
    return e->nr;
}

