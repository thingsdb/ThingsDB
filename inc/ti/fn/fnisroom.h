#include <ti/fn/fn.h>

static int do__f_is_room(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    _Bool is_room;

    if (fn_nargs("is_room", DOC_IS_ROOM, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    is_room = ti_val_is_room(query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_room);

    return e->nr;
}
