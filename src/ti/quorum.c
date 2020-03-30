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
    ti_quorum_t * quorum;

    quorum = malloc(sizeof(ti_quorum_t));
    if (!quorum)
        return NULL;

    quorum->accepted = 0;
    quorum->rejected = 0;
    quorum->collisions = 0;
    quorum->requests = ti.nodes->vec->n;
    quorum->quorum = ti_nodes_quorum();
    quorum->win_collision = 1;  /* true */
    quorum->diff_requests = 0;

    /* sets accept and reject threshold */
    (void) ti_quorum_shrink_one(quorum);

    quorum->data = data;
    quorum->cb_ = cb;

    return quorum;
}

void ti_quorum_go(ti_quorum_t * quorum)
{
    uint8_t n = quorum->accepted + quorum->rejected + quorum->collisions;

    if (quorum->cb_)
    {
        if (quorum->requests < quorum->quorum)
        {
            quorum->cb_(quorum->data, false);
            quorum->cb_ = NULL;
        }
        else if (quorum->accepted == quorum->accept_threshold)
        {
            quorum->cb_(quorum->data, true);
            quorum->cb_ = NULL;
        }
        else if (quorum->rejected == quorum->accept_threshold)
        {
            quorum->cb_(quorum->data, false);
            quorum->cb_ = NULL;
        }
    }

    if (n < quorum->requests)
        return;

    if (quorum->cb_)
        quorum->cb_(
            quorum->data,
            quorum->diff_requests < 0
            ? false
            : quorum->collisions > quorum->diff_requests &&
              quorum->accepted == quorum->rejected
            ? quorum->win_collision
            : quorum->accepted > quorum->rejected);

    free(quorum);
}

void ti_quorum_req_cb(ti_req_t * req, ex_enum status)
{
    ti_quorum_t * quorum = req->data;
    uint8_t tp = status == 0 ? req->pkg_res->tp : TI_PROTO_NODE_ERR;

    switch (tp)
    {
    case TI_PROTO_NODE_RES_ACCEPT:
        ++quorum->accepted;
        break;
    case TI_PROTO_NODE_ERR_REJECT:
        ++quorum->rejected;
        break;
    case TI_PROTO_NODE_ERR_COLLISION:
    {
        mp_unp_t up;
        mp_obj_t n;

        mp_unp_init(&up, req->pkg_res->data, req->pkg_res->n);

        if (mp_next(&up, &n) == MP_U64 && n.via.u64 != quorum->requests)
        {
            if (quorum->requests > n.via.u64)
                ++quorum->diff_requests;
            else
                --quorum->diff_requests;
        }
        else if (req->stream->via.node->id < ti.node->id)
        {
            quorum->win_collision = 0;  /* false */
        }

        ++quorum->collisions;
        break;
    }
    default:
        if (status == 0)
            ti_pkg_log(req->pkg_res);
        (void) ti_quorum_shrink_one(quorum);
    }
    ti_req_destroy(req);
    ti_quorum_go(quorum);
}
