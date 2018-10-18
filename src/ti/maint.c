/*
 * maint.c
 */
#include <nodes.h>
#include <props.h>
#include <stdlib.h>
#include <thingsdb.h>
#include <ti/event.h>
#include <ti/maint.h>
#include <ti/proto.h>
#include <ti/store.h>
#include <ti/things.h>
#include <util/queue.h>
#include <util/qpx.h>

const int ti__maint_repeat = 3000;
const int ti__maint_reg_timeout = 10;

static void ti__maint_timer_cb(uv_timer_t * timer);
static int ti__maint_reg(ti_maint_t * maint);
static void ti__maint_on_reg_cb(ti_prom_t * prom);
static void ti__maint_wait(ti_maint_t * maint);
static void ti__maint_work(uv_work_t * work);
static void ti__maint_work_finish(uv_work_t * work, int status);
inline static int ti__maint_cmp(uint8_t a, uint8_t b, uint8_t n, uint64_t i);


ti_maint_t * ti_maint_new(void)
{
    ti_maint_t * maint = (ti_maint_t *) malloc(sizeof(ti_maint_t));
    if (!maint)
        return NULL;
    maint->timer.data = maint;
    maint->work.data = maint;
    maint->status = TI_MAINT_STAT_NIL;
    return maint;
}

int ti_maint_start(ti_maint_t * maint)
{
    maint->status = TI_MAINT_STAT_READY;
    maint->last_commit = thingsdb_get()->events->commit_id;
    return (uv_timer_init(tingsdb_loop(), &maint->timer) ||
            uv_timer_start(
                &maint->timer,
                ti__maint_timer_cb,
                ti__maint_repeat,
                ti__maint_repeat));
}

void ti_maint_stop(ti_maint_t * maint)
{
    if (maint->status > TI_MAINT_STAT_NIL)
    {
        uv_timer_stop(&maint->timer);
        uv_close((uv_handle_t *) &maint->timer, NULL);
    }
}

static void ti__maint_timer_cb(uv_timer_t * timer)
{
    ti_maint_t * maint = (ti_maint_t *) timer->data;
    thingsdb_t * thingsdb = thingsdb_get();
    uint64_t commit_id = thingsdb->events->commit_id;

    if (maint->status == TI_MAINT_STAT_WAIT)
    {
        ti__maint_wait(maint);
        return;
    }

    if (maint->status != TI_MAINT_STAT_READY ||
        maint->last_commit == commit_id ||
        !ti_nodes_has_quorum(thingsdb->nodes)) return;

    for (vec_each(thingsdb->nodes, ti_node_t, node))
    {
        if (node == thingsdb->node) continue;

        if (node->status == TI_NODE_STAT_MAINT)
        {
            log_debug("node '%s' is in maintenance mode", node->addr);
            return;
        }

        if (node->maintn < thingsdb->node->maintn) continue;

        if (node->maintn > thingsdb->node->maintn)
        {
            log_debug("node '%s' has a higher maintenance counter", node->addr);
            thingsdb->node->maintn++;
            return;
        }

        if (ti__maint_cmp(
                node->id,
                thingsdb->node->id,
                thingsdb->nodes->n,
                commit_id) > 0)
        {
            log_debug("node '%s' wins on the maintenance counter", node->addr);
            thingsdb->node->maintn++;
            return;
        }
    }

    if (ti__maint_reg(maint))
    {
        log_error("failed to write maintenance request");
    }
}

static int ti__maint_reg(ti_maint_t * maint)
{
    thingsdb_t * thingsdb = thingsdb_get();
    maint->status = TI_MAINT_STAT_REG;
    ti_prom_t * prom = ti_prom_new(
            thingsdb->nodes->n - 1,
            maint,
            ti__maint_on_reg_cb);
    qpx_packer_t * xpkg = qpx_packer_create(8);
    if (!prom ||
        !xpkg ||
        qp_add_int64(xpkg, (int64_t) thingsdb->node->maintn)) goto failed;

    ti_pkg_t * pkg = qpx_packer_pkg(xpkg, TI_BACK_EVENT_UPD);

    for (vec_each(thingsdb->nodes, ti_node_t, node))
    {
        if (node == thingsdb->node) continue;
        if (node->status <= TI_NODE_STAT_CONNECTED || ti_req(
                node,
                pkg,
                ti__maint_reg_timeout,
                prom,
                ti_prom_req_cb))
        {
            prom->sz--;
        }
    }

    free(prom->sz ? NULL : pkg);

    ti_prom_go(prom);

    return 0;

failed:
    maint->status = TI_MAINT_STAT_READY;
    free(prom);
    qpx_packer_destroy(xpkg);
    return -1;
}

static void ti__maint_on_reg_cb(ti_prom_t * prom)
{
    ti_maint_t * maint = (ti_maint_t *) prom->data;
    _Bool accept = 1;

    for (size_t i = 0; i < prom->n; i++)
    {
        ti_prom_res_t * res = &prom->res[i];
        ti_req_t * req = (ti_req_t *) res->handle;
        free(i ? NULL : req->pkg_req);
        if (res->status || req->pkg_res->tp == TI_PROTO_REJECT)
        {
            accept = 0;
        }
        ti_req_destroy(req);
    }

    free(prom);

    if (accept)
    {
        thingsdb_get()->node->status = TI_NODE_STAT_MAINT;
        maint->status = TI_MAINT_STAT_WAIT;
        ti__maint_wait(maint);
    }
}

static void ti__maint_wait(ti_maint_t * maint)
{
    thingsdb_t * thingsdb = thingsdb_get();
    if (thingsdb->events->queue->n)
    {
        log_debug("wait until the event queue is empty (%zd)",
                thingsdb->events->queue->n);
        return;
    }
    maint->status = TI_MAINT_STAT_BUSY;
    uv_queue_work(
            thingsdb->loop,
            &maint->work,
            ti__maint_work,
            ti__maint_work_finish);
}

static void ti__maint_work(uv_work_t * work)
{
    ti_maint_t * maint = (ti_maint_t *) work->data;
    thingsdb_t * thingsdb = thingsdb_get();
    uv_mutex_lock(&thingsdb->events->lock);

    log_debug("maintenance job: start storing data");

    for (vec_each(thingsdb->dbs, ti_db_t, db))
    {
        if (ti_things_gc(db->things, db->root))
        {
            log_error("garbage collection of things has failed");
            continue;
        }
    }

    log_debug("maintenance job: start storing data");

    ti_store();

    maint->last_commit = thingsdb->events->commit_id;

    log_debug("maintenance job: finished storing data");

    uv_mutex_unlock(&thingsdb->events->lock);
}

static void ti__maint_work_finish(uv_work_t * work, int status)
{
    ti_maint_t * maint = (ti_maint_t *) work->data;
    thingsdb_get()->node->status = TI_NODE_STAT_READY;
    maint->status = TI_MAINT_STAT_READY;
    if (status)
    {
        log_error(uv_strerror(status));
    }
}

inline static int ti__maint_cmp(uint8_t a, uint8_t b, uint8_t n, uint64_t i)
{
    return ((i + a) % n) - ((i + b) % n);
}
