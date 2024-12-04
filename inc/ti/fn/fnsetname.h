#include <ti/fn/fn.h>


static int do__f_set_name(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_room_t * room;
    ti_task_t * task;

    if (!ti_val_is_room(query->rval))
        return fn_call_try("set_name", query, nd, e);

    if (fn_nargs("set_name", DOC_ROOM_SET_NAME, 1, nargs, e))
        return e->nr;

    room = (ti_room_t *) query->rval;
    query->rval = NULL;

    if (!room->id)
    {
        ex_set(e, EX_OPERATION,
            "names can only be assigned to stored rooms (a room with an Id)");
        goto done;
    }

    /* we must have a collection on the query, as a room with Id only exists
     * in a collection scope */
    assert(query->collection);

    if (ti_do_statement(query, nd->children, e))
        goto done;

    if (ti_val_is_str(query->rval))
    {
        ti_room_t * prev;
        ti_name_t * name;
        ti_raw_t * raw = (ti_raw_t *) query->rval;
        if (!ti_name_is_valid_strn((const char *) raw->data, raw->n))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "room name must follow the naming rules"DOC_NAMES);
            goto done;
        }
        prev = ti_collection_room_by_strn(
            query->collection,
            (const char *) raw->data,
            raw->n);

        if (prev == room)
        {
            /* name is already set, do nothing */
            ti_val_unsafe_drop(query->rval);
            query->rval = (ti_val_t *) ti_nil_get();
            goto done;
        }
        if (prev)
        {
            ex_set(e, EX_LOOKUP_ERROR,
                    "room `%.*s` already exists",
                    raw->n,
                    (const char *) raw->data);
            goto done;
        }
        name = ti_names_get((const char *) raw->data, raw->n);
        ti_val_unsafe_drop(query->rval);
        query->rval = (ti_val_t *) ti_nil_get();
        if (!name || ti_room_set_name(room, name))
        {
            ex_set_mem(e);
            ti_name_drop(name);
            goto done;
        }
        ti_decref(name);
    }
    else if (ti_val_is_nil(query->rval))
    {
        ti_room_unset_name(room);
    }
    else
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `set_name` expects argument 1 to be of "
            "type `"TI_VAL_STR_S"` or `"TI_VAL_NIL_S"` but "
            "got type `%s` instead"DOC_ROOM_SET_NAME,
            ti_val_str(query->rval));
        goto done;
    }
    task = ti_task_get_task(query->change, query->collection->root);
    if (!task || ti_task_set_name(task, room))
        ex_set_mem(e);

done:
    ti_val_unsafe_drop((ti_val_t *) room);
    return e->nr;
}
