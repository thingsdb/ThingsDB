#include <ti/fn/fn.h>

#define RENAME_DOC_ TI_SEE_DOC("#rename")

static int do__f_rename(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    const int nargs = langdef_nd_n_function_params(nd);
    ti_thing_t * thing;
    cleri_node_t * from_nd, * to_nd;
    ti_raw_t * from_raw = NULL;
    ti_raw_t * to_raw;
    ti_name_t * from_name, * to_name;
    ti_task_t * task;

    thing = (ti_thing_t *) ti_query_val_pop(query);

    if (thing->tp != TI_VAL_THING)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `rename`"RENAME_DOC_,
                ti_val_str((ti_val_t *) thing));
        goto done;
    }

    if (nargs != 2)
    {
        ex_set(e, EX_BAD_DATA,
                "function `rename` takes 2 arguments but %d %s given"
                RENAME_DOC_, nargs, nargs == 1 ? "was" : "were");
        goto done;
    }

    if (!thing->id)
    {
        ex_set(e, EX_BAD_DATA,
                "function `rename` can only be used on things with an id > 0; "
                "things which are assigned automatically receive an id"
                RENAME_DOC_);
        goto done;
    }

    if (ti_scope_in_use_thing(query->scope->prev, thing))
    {
        ex_set(e, EX_BAD_DATA,
                "cannot use `rename` while thing "TI_THING_ID" is in use"
                RENAME_DOC_, thing->id);
        goto done;
    }

    from_nd = nd
            ->children->node;
    to_nd = nd
            ->children->next->next->node;

    if (ti_do_scope(query, from_nd, e))
        goto done;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `rename` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead"RENAME_DOC_,
                ti_val_str(query->rval));
        goto done;
    }

    from_raw = (ti_raw_t *) query->rval;
    query->rval = NULL;

    from_name = ti_names_weak_get((const char *) from_raw->data, from_raw->n);
    if (!from_name)
    {
        if (!ti_name_is_valid_strn((const char *) from_raw->data, from_raw->n))
        {
            ex_set(e, EX_BAD_DATA,
                    "function `rename` expects argument 1 to be a valid name"
                    TI_SEE_DOC("#names"));
        }
        else
        {
            ex_set(e, EX_INDEX_ERROR,
                    "thing "TI_THING_ID" has no property `%.*s`",
                    thing->id,
                    (int) from_raw->n, (const char *) from_raw->data);
        }
        goto done;
    }

    if (ti_do_scope(query, to_nd, e))
        goto done;

    if (!ti_val_is_valid_name(query->rval))
    {
        if (ti_val_is_raw(query->rval))
        {
            ex_set(e, EX_BAD_DATA,
                    "function `rename` expects argument 2 to be a valid name, "
                    TI_SEE_DOC("#names"));
        }
        else
        {
            ex_set(e, EX_BAD_DATA,
                    "function `rename` expects argument 2 to be of "
                    "type `"TI_VAL_RAW_S"` but got type `%s` instead"
                    RENAME_DOC_, ti_val_str(query->rval));
        }
        goto done;
    }

    to_raw = (ti_raw_t *) query->rval;
    to_name = ti_names_get((const char *) to_raw->data, to_raw->n);
    if (!to_name)
        goto alloc_err;

    if (!ti_thing_rename(thing, from_name, to_name))
    {
        ti_name_drop(to_name);
        ex_set(e, EX_INDEX_ERROR,
                "thing "TI_THING_ID" has no property `%.*s`",
                thing->id,
                (int) from_raw->n, (const char *) from_raw->data);
        goto done;
    }

    task = ti_task_get_task(query->ev, thing, e);
    if (!task)
        goto done;

    if (ti_task_add_rename(task, from_raw, to_raw))
        goto alloc_err;

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    goto done;

alloc_err:
    ex_set_mem(e);

done:
    ti_val_drop((ti_val_t *) from_raw);
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}
