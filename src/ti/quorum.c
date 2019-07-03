/*
 * ti/quorum.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/nodes.h>
#include <ti/quorum.h>
#include <ti/proto.h>

ti_quorum_t * ti_quorum_new(ti_quorum_cb cb, void * data)
{
    assert (ti()->nodes->vec->n);

    ti_quorum_t * quorum;
    uint8_t nnodes = ti()->nodes->vec->n;
    uint8_t sz = nnodes - 1;

    quorum = malloc(sizeof(ti_quorum_t));
    if (!quorum)
        return NULL;

    quorum->n = 0;
    quorum->accepted = 0;
    quorum->sz = sz;
    quorum->quorum = ti_nodes_quorum();
    quorum->reject_threshold = nnodes - quorum->quorum;
    quorum->data = data;
    quorum->cb_ = cb;

    return quorum;
}

void ti_quorum_go(ti_quorum_t * quorum)
{
    if (quorum->cb_)
    {
        if (quorum->accepted == quorum->quorum)
        {
            quorum->cb_(quorum->data, true);
            quorum->cb_ = NULL;
        }
        else if (quorum->n - quorum->accepted == quorum->reject_threshold)
        {
            quorum->cb_(quorum->data, false);
            quorum->cb_ = NULL;
        }
    }

    if (quorum->n == quorum->sz)
    {
        if (quorum->cb_)
            quorum->cb_(quorum->data, false);

        free(quorum);
    }
}

void ti_quorum_req_cb(ti_req_t * req, ex_enum status)
{
    ti_quorum_t * quorum = req->data;
    if (status == 0)
    {
        switch (req->pkg_res->tp)
        {
        case TI_PROTO_NODE_RES_EVENT_ID:
        case TI_PROTO_NODE_RES_AWAY:
            ++quorum->accepted;
            break;
        case TI_PROTO_NODE_ERR_EVENT_ID:
        case TI_PROTO_NODE_ERR_AWAY:
            break;
        default:
            ti_pkg_log(req->pkg_res);
        }
    }
    ti_req_destroy(req);
    ti_quorum_go(quorum);
}
