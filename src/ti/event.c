/*
 * event.c
 */
#include <assert.h>
#include <qpack.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/db.h>
#include <ti/event.h>
#include <ti/ex.h>
#include <ti/node.h>
#include <ti/proto.h>
#include <ti/task.h>
#include <util/imap.h>
#include <util/logger.h>
#include <util/qpx.h>


ti_event_t * ti_event_create(ti_event_tp_enum tp)
{
    ti_event_t * ev = malloc(sizeof(ti_event_t));
    if (!ev)
        return NULL;

    ev->status = TI_EVENT_STAT_NEW;
    ev->target = NULL;
    ev->tp = tp;
    ev->tasks = imap_create();

    if (!ev->tasks)
    {
        free(ev);
        return NULL;
    }

    return ev;
}

void ti_event_destroy(ti_event_t * ev)
{
    if (!ev)
        return;

    ti_db_drop(ev->target);

    if (ev->tp == TI_EVENT_TP_SLAVE)
        ti_node_drop(ev->via.node);

    imap_destroy(ev->tasks, (imap_destroy_cb) ti_task_destroy);

    free(ev);
}

void ti_event_cancel(ti_event_t * ev)
{
    ti_pkg_t * pkg;
    vec_t * vec_nodes = ti()->nodes->vec;
    qpx_packer_t * packer = qpx_packer_create(9, 0);
    if (!packer)
    {
        log_error(EX_ALLOC_S);
        return;
    }

    (void) qp_add_int64(packer, ev->id);

    pkg = qpx_packer_pkg(packer, TI_PROTO_NODE_EVENT_CANCEL);

    for (vec_each(vec_nodes, ti_node_t, node))
    {
        ti_pkg_t * dup;

        if (node == ti()->node || node->status <= TI_NODE_STAT_CONNECTING)
            continue;

        dup = ti_pkg_dup(pkg);
        if (!dup)
        {
            log_error(EX_ALLOC_S);
            continue;
        }

        if (ti_node_write(node, dup))
        {
            log_error(EX_INTERNAL_S);
            free(dup);
        }
    }
    free(pkg);
}
