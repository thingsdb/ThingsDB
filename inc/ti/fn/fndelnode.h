#include <ti/fn/fn.h>

static int do__f_del_node(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_node_t * node;
    uint32_t node_id;
    ti_task_t * task;

    if (fn_not_thingsdb_scope("del_node", query, e) ||
        fn_nargs("del_node", DOC_DEL_NODE, 1, nargs, e) ||
        ti_do_statement(query, nd->children->node, e) ||
        fn_arg_int("del_node", DOC_DEL_NODE, 1, query->rval, e))
        return e->nr;

    node_id = (uint32_t) VINT(query->rval);

    node = ti_nodes_node_by_id(node_id);
    if (!node)
    {
        ex_set(e, EX_LOOKUP_ERROR, "node with id %"PRId64" not found",
                VINT(query->rval));
        return e->nr;
    }

    if (node->status >= TI_NODE_STAT_SYNCHRONIZING)
    {
        ex_set(e, EX_NODE_ERROR,
                TI_NODE_ID" is still active; shutdown the node before removal"
                DOC_DEL_NODE, node->id);
        return e->nr;
    }

    assert (node != ti.node);

    ti_nodes_del_node(node_id);
    query->ev->flags |= TI_EVENT_FLAG_SAVE;

    task = ti_task_get_task(query->ev, ti.thing0, e);
    if (!task)
        return e->nr;

    if (ti_task_add_del_node(task, node_id))
        ex_set_mem(e);  /* task cleanup is not required */

    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
