#include <ti/fn/fn.h>

#define DEL_DOC_ TI_SEE_DOC("#del")

static int do__f_del(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    cleri_node_t * name_nd;
    ti_task_t * task;
    ti_name_t * name;
    ti_raw_t * rname;
    ti_thing_t * thing = (ti_thing_t *) ti_query_val_pop(query);

    if (thing->tp != TI_VAL_THING)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `del`"DEL_DOC_,
                ti_val_str((ti_val_t *) thing));
        goto done;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `del` takes 1 argument but %d were given"DEL_DOC_,
                nargs);
        goto done;
    }

    if (!thing->id)
    {
        ex_set(e, EX_BAD_DATA,
                "function `del` can only be used on things with an id > 0; "
                "things which are assigned automatically receive an id"
                DEL_DOC_);
        goto done;
    }

    if (ti_scope_in_use_thing(query->scope->prev, thing))
    {
        ex_set(e, EX_BAD_DATA,
                "cannot use `del` while thing "TI_THING_ID" is in use"DEL_DOC_,
                thing->id);
        goto done;
    }

    name_nd = nd->children->node;

    if (ti_do_scope(query, name_nd, e))
        goto done;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `del` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead"DEL_DOC_,
                ti_val_str(query->rval));
        goto done;
    }

    rname = (ti_raw_t *) query->rval;
    name = ti_names_weak_get((const char *) rname->data, rname->n);

    if (!name || !ti_thing_del(thing, name))
    {
        if (ti_name_is_valid_strn((const char *) rname->data, rname->n))
        {
            ex_set(e, EX_INDEX_ERROR,
                    "thing "TI_THING_ID" has no property `%.*s`",
                    thing->id,
                    (int) rname->n, (const char *) rname->data);
        }
        else
        {
            ex_set(e, EX_BAD_DATA,
                    "function `del` expects argument 1 to be a valid name"
                    TI_SEE_DOC("#names"));
        }
        goto done;
    }

    task = ti_task_get_task(query->ev, thing, e);
    if (!task)
        goto done;

    if (ti_task_add_del(task, rname))
    {
        ex_set_alloc(e);
        goto done;
    }

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

done:
    ti_val_drop((ti_val_t *) thing);

    return e->nr;
}
