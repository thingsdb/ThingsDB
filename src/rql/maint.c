/*
 * maint.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <rql/maint.h>
#include <rql/event.h>
#include <rql/store.h>
#include <rql/props.h>
#include <rql/elems.h>
#include <rql/proto.h>
#include <rql/nodes.h>
#include <util/queue.h>
#include <util/qpx.h>

const int rql__maint_repeat = 3000;
const int rql__maint_reg_timeout = 10;

static void rql__maint_timer_cb(uv_timer_t * timer);
static int rql__maint_reg(rql_maint_t * maint);
static void rql__maint_on_reg_cb(rql_prom_t * prom);
static void rql__maint_wait(rql_maint_t * maint);
static void rql__maint_work(uv_work_t * work);
static void rql__maint_work_finish(uv_work_t * work, int status);
inline static int rql__maint_cmp(uint8_t a, uint8_t b, uint8_t n, uint64_t i);


rql_maint_t * rql_maint_new(rql_t * rql)
{
    rql_maint_t * maint = (rql_maint_t *) malloc(sizeof(rql_maint_t));
    if (!maint) return NULL;
    maint->rql = rql;
    maint->timer.data = maint;
    maint->work.data = maint;
    maint->status = RQL_MAINT_STAT_NIL;
    return maint;
}

int rql_maint_start(rql_maint_t * maint)
{
    maint->status = RQL_MAINT_STAT_READY;
    maint->last_commit = maint->rql->events->commit_id;
    return (uv_timer_init(&maint->rql->loop, &maint->timer) ||
            uv_timer_start(
                &maint->timer,
                rql__maint_timer_cb,
                rql__maint_repeat,
                rql__maint_repeat));
}

void rql_maint_stop(rql_maint_t * maint)
{
    if (maint->status > RQL_MAINT_STAT_NIL)
    {
        uv_timer_stop(&maint->timer);
        uv_close((uv_handle_t *) &maint->timer, NULL);
    }
}

static void rql__maint_timer_cb(uv_timer_t * timer)
{
    rql_maint_t * maint = (rql_maint_t *) timer->data;
    rql_t * rql = maint->rql;
    uint64_t commit_id = rql->events->commit_id;

    if (maint->status == RQL_MAINT_STAT_WAIT)
    {
        rql__maint_wait(maint);
        return;
    }

    if (maint->status != RQL_MAINT_STAT_READY ||
        maint->last_commit == commit_id ||
        !rql_nodes_has_quorum(rql->nodes)) return;

    for (vec_each(rql->nodes, rql_node_t, node))
    {
        if (node == rql->node) continue;

        if (node->status == RQL_NODE_STAT_MAINT)
        {
            log_debug("node '%s' is in maintenance mode", node->addr);
            return;
        }

        if (node->maintn < rql->node->maintn) continue;

        if (node->maintn > rql->node->maintn)
        {
            log_debug("node '%s' has a higher maintenance counter", node->addr);
            rql->node->maintn++;
            return;
        }

        if (rql__maint_cmp(
                node->id,
                rql->node->id,
                rql->nodes->n,
                commit_id) > 0)
        {
            log_debug("node '%s' wins on the maintenance counter", node->addr);
            rql->node->maintn++;
            return;
        }
    }

    if (rql__maint_reg(maint))
    {
        log_error("failed to write maintenance request");
    }
}

static int rql__maint_reg(rql_maint_t * maint)
{
    maint->status = RQL_MAINT_STAT_REG;
    rql_prom_t * prom = rql_prom_new(
            maint->rql->nodes->n - 1,
            maint,
            rql__maint_on_reg_cb);
    qpx_packer_t * xpkg = qpx_packer_create(8);
    if (!prom ||
        !xpkg ||
        qp_add_int64(xpkg, (int64_t) maint->rql->node->maintn)) goto failed;

    rql_pkg_t * pkg = qpx_packer_pkg(xpkg, RQL_BACK_EVENT_UPD);

    for (vec_each(maint->rql->nodes, rql_node_t, node))
    {
        if (node == maint->rql->node) continue;
        if (node->status <= RQL_NODE_STAT_CONNECTED || rql_req(
                node,
                pkg,
                rql__maint_reg_timeout,
                prom,
                rql_prom_req_cb))
        {
            prom->sz--;
        }
    }

    free(prom->sz ? NULL : pkg);

    rql_prom_go(prom);

    return 0;

failed:
    maint->status = RQL_MAINT_STAT_READY;
    free(prom);
    qpx_packer_destroy(xpkg);
    return -1;
}

static void rql__maint_on_reg_cb(rql_prom_t * prom)
{
    rql_maint_t * maint = (rql_maint_t *) prom->data;
    _Bool accept = 1;

    for (size_t i = 0; i < prom->n; i++)
    {
        rql_prom_res_t * res = &prom->res[i];
        rql_req_t * req = (rql_req_t *) res->handle;
        free(i ? NULL : req->pkg_req);
        if (res->status || req->pkg_res->tp == RQL_PROTO_REJECT)
        {
            accept = 0;
        }
        rql_req_destroy(req);
    }

    free(prom);

    if (accept)
    {
        maint->rql->node->status = RQL_NODE_STAT_MAINT;
        maint->status = RQL_MAINT_STAT_WAIT;
        rql__maint_wait(maint);
    }
}

static void rql__maint_wait(rql_maint_t * maint)
{
    if (maint->rql->events->queue->n)
    {
        log_debug("wait until the event queue is empty (%zd)",
                maint->rql->events->queue->n);
        return;
    }
    maint->status = RQL_MAINT_STAT_BUSY;
    uv_queue_work(
            &maint->rql->loop,
            &maint->work,
            rql__maint_work,
            rql__maint_work_finish);
}

static void rql__maint_work(uv_work_t * work)
{
    rql_maint_t * maint = (rql_maint_t *) work->data;

    uv_mutex_lock(&maint->rql->events->lock);

    log_debug("maintenance job: start storing data");

    for (vec_each(maint->rql->dbs, rql_db_t, db))
    {
        if (rql_elems_gc(db->elems, db->root))
        {
            log_error("garbage collection of elements has failed");
            continue;
        }

        if (rql_props_gc(db->props))
        {
            log_error("garbage collection of props has failed");
            continue;
        }
    }

    log_debug("maintenance job: start storing data");

    rql_store(maint->rql);

    maint->last_commit = maint->rql->events->commit_id;

    log_debug("maintenance job: finished storing data");

    uv_mutex_unlock(&maint->rql->events->lock);
}

static void rql__maint_work_finish(uv_work_t * work, int status)
{
    rql_maint_t * maint = (rql_maint_t *) work->data;
    maint->rql->node->status = RQL_NODE_STAT_READY;
    maint->status = RQL_MAINT_STAT_READY;
    if (status)
    {
        log_error(uv_strerror(status));
    }
}

inline static int rql__maint_cmp(uint8_t a, uint8_t b, uint8_t n, uint64_t i)
{
    return ((i + a) % n) - ((i + b) % n);
}
