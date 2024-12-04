#include <ti/fn/fn.h>

static int do__f_room(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    /* Rooms can be created in every scope, but they have no effect in
     * any scope other than a collection scope. This is because they must get
     * an ID before anyone can listen to a room and an Id is only created when
     * a room is assigned to a collection;
     */
    int nargs = fn_get_nargs(nd);

    if (fn_nargs_max("room", DOC_ROOM, 1, nargs, e))
        return e->nr;

    if (nargs == 1)
    {

        ti_room_t * room;

        if (ti_do_statement(query, nd->children, e) ||
            ti_val_is_room(query->rval))
            return e->nr;


        if (ti_val_is_int(query->rval))
        {
            int64_t id = VINT(query->rval);
            room = ti_query_room_from_id(query, id, e);
        }
        else if (ti_val_is_str(query->rval))
        {
            ti_str_t * str = (ti_str_t *) query->rval;
            room = ti_query_room_from_strn(query, str->str, str->n, e);
        }
        else
        {
            ex_set(e, EX_TYPE_ERROR,
                    "cannot convert type `%s` to `"TI_VAL_ROOM_S"`",
                    ti_val_str(query->rval));
            return e->nr;
        }

        if (!room)
            return e->nr;

        ti_val_unsafe_drop(query->rval);
        query->rval = (ti_val_t *) room;

        return e->nr;
    }

    query->rval = (ti_val_t *) ti_room_create(0, query->collection);

    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
