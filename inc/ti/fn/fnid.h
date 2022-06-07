#include <ti/fn/fn.h>


static int do__f_id_thing(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_thing_t * thing;

    if (fn_nargs("id", DOC_THING_ID, 0, nargs, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;
    query->rval = thing->id
            ? (ti_val_t *) ti_vint_create((int64_t) thing->id)
            : (ti_val_t *) ti_nil_get();

    if (!query->rval)
        ex_set_mem(e);

    ti_val_unsafe_drop((ti_val_t *) thing);
    return e->nr;
}

static int do__f_id_room(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_room_t * room;

    if (fn_nargs("id", DOC_ROOM_ID, 0, nargs, e))
        return e->nr;

    room = (ti_room_t *) query->rval;
    query->rval = room->id
            ? (ti_val_t *) ti_vint_create((int64_t) room->id)
            : (ti_val_t *) ti_nil_get();

    if (!query->rval)
        ex_set_mem(e);

    ti_val_unsafe_drop((ti_val_t *) room);
    return e->nr;
}

static int do__f_id_task(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_vtask_t * vtask;

    if (fn_nargs("id", DOC_TASK_ID, 0, nargs, e))
        return e->nr;

    vtask = (ti_vtask_t *) query->rval;
    query->rval = vtask->id
            ? (ti_val_t *) ti_vint_create((int64_t) vtask->id)
            : (ti_val_t *) ti_nil_get();

    if (!query->rval)
        ex_set_mem(e);

    ti_val_unsafe_drop((ti_val_t *) vtask);
    return e->nr;
}

static inline int do__f_id(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    return ti_val_is_thing(query->rval)
            ? do__f_id_thing(query, nd, e)
            : ti_val_is_room(query->rval)
            ? do__f_id_room(query, nd, e)
            : ti_val_is_task(query->rval)
            ? do__f_id_task(query, nd, e)
            : fn_call_try("id", query, nd, e);
}
