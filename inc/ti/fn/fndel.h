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
    ti_thing_t * thing;

    if (!ti_val_isthing(query->rval))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `del`"DEL_DOC_,
                ti_val_str((ti_val_t *) thing));
        return e->nr;
    }

    thing = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `del` takes 1 argument but %d were given"DEL_DOC_,
                nargs);
        goto done;
    }

    if (thing->flags & TI_VFLAG_LOCK)
    {
        ex_set(e, EX_BAD_DATA,
            "cannot delete properties while "TI_THING_ID" is in use"DEL_DOC_,
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

    if (thing->id)
    {
        task = ti_task_get_task(query->ev, thing, e);
        if (!task)
            goto done;

        if (ti_task_add_del(task, rname))
        {
            ex_set_mem(e);
            goto done;
        }
    }

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

done:
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}
