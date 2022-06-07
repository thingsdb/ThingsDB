#include <ti/fn/fn.h>


static int do__f_emit(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    int sargs = 1;
    int deep = query->qbind.deep;
    ti_room_t * room;
    ti_raw_t * revent;
    vec_t * vec = NULL;
    cleri_node_t * child = nd->children;

    if (!ti_val_is_room(query->rval))
        return fn_call_try("emit", query, nd, e);

    if (fn_nargs_min("emit", DOC_ROOM_EMIT, 1, nargs, e))
        return e->nr;

    room = (ti_room_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, child, e))
        goto fail0;

    if (ti_val_is_int(query->rval))
    {
        int64_t deepi;

        if (nargs == 1)
        {
            ex_set(e, EX_NUM_ARGUMENTS,
                "function `emit` requires at least 2 arguments "
                "when `deep` is used but 1 was given "DOC_ROOM_EMIT);
            goto fail0;
        }

        ++sargs;

        deepi = VINT(query->rval);

        if (deepi < 0 || deepi > TI_MAX_DEEP)
        {
            ex_set(e, EX_VALUE_ERROR,
                    "expecting a `deep` value between 0 and %d "
                    "but got %"PRId64" instead",
                    TI_MAX_DEEP, deepi);
            goto fail0;
        }

        deep = (int) deepi;

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;

        if (ti_do_statement(query, (child = child->next->next), e))
            goto fail0;
    }

    if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `emit` expects the `event` argument to be of "
                "type `"TI_VAL_STR_S"` but got type `%s` instead"
                DOC_ROOM_EMIT,
                ti_val_str(query->rval));
        goto fail0;
    }

    revent = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (revent->n < TI_ROOM_ENAME_MIN || revent->n > TI_ROOM_ENAME_MAX)
    {
        ex_set(e, EX_VALUE_ERROR,
            "function `emit` expects the `event` argument to be between %d "
            "and %d characters",
            TI_ROOM_ENAME_MIN, TI_ROOM_ENAME_MAX);
        goto fail1;
    }

    if (nargs > sargs)
    {
        child = child->next->next;

        vec = vec_new(nargs-sargs);
        if (!vec)
        {
            ex_set_mem(e);
            goto fail1;
        }

        do
        {
            if (ti_do_statement(query, child, e))
                goto fail2;

            VEC_push(vec, query->rval);
            query->rval = NULL;
        }
        while (child->next && (child = child->next->next));
    }

    if (room->id && ti_room_emit_raw(room, query, revent, vec, deep))
        ex_set_mem(e);

    query->rval = (ti_val_t *) ti_nil_get();

fail2:
    vec_destroy(vec, (vec_destroy_cb) ti_val_unsafe_drop);
fail1:
    ti_val_unsafe_drop((ti_val_t *) revent);
fail0:
    ti_val_unsafe_drop((ti_val_t *) room);
    return e->nr;
}
