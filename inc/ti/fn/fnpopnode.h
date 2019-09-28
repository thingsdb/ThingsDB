#include <ti/fn/fn.h>

static int do__f_pop_node(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_node_t * node;
    uint8_t node_id;
    ti_task_t * task;

    if (fn_not_thingsdb_scope("pop_node", query, e) ||
        fn_nargs("pop_node", DOC_POP_NODE, 0, nargs, e))
        return e->nr;

    node = vec_last(ti()->nodes->vec);
    assert (node);

    if (node->status >= TI_NODE_STAT_SYNCHRONIZING)
    {
        ex_set(e, EX_NODE_ERROR,
                TI_NODE_ID" is still active, shutdown the node before removal"
                DOC_POP_NODE, node->id);
        return e->nr;
    }

    assert (node != ti()->node);
    node_id = node->id;

    ti_nodes_pop_node();
    query->ev->flags |= TI_EVENT_FLAG_SAVE;

    task = ti_task_get_task(query->ev, ti()->thing0, e);
    if (!task)
        return e->nr;

    if (ti_task_add_pop_node(task, node_id))
        ex_set_mem(e);  /* task cleanup is not required */

    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}
