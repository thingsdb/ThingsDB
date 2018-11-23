/*
 * ti/wareq.c
 */
#include <ti/wareq.h>
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/dbs.h>
#include <ti/proto.h>
#include <ti/fwd.h>
#include <util/qpx.h>

static void wareq__destroy(ti_wareq_t * wareq);
static void wareq__destroy_cb(uv_async_t * task);
static void wareq__task_cb(uv_async_t * task);
static int wareq__fwd_wareq(ti_wareq_t * wareq, uint64_t thing_id);
static void wareq__fwd_wareq_cb(ti_req_t * req, ex_enum status);


ti_wareq_t * ti_wareq_create(ti_stream_t * stream)
{
    ti_wareq_t * wareq = malloc(sizeof(ti_wareq_t));
    if (!wareq)
        return NULL;

    wareq->stream = ti_grab(stream);
    wareq->thing_ids = NULL;
    wareq->target = NULL;
    wareq->task = malloc(sizeof(uv_async_t));

    if (!wareq->task || uv_async_init(ti()->loop, wareq->task, wareq__task_cb))
    {
        wareq__destroy(wareq);
        return NULL;
    }

    wareq->task->data = wareq;

    return wareq;
}

void ti_wareq_destroy(ti_wareq_t * wareq)
{
    if (!wareq)
        return;
    assert (wareq->task);
    uv_close((uv_handle_t *) wareq->task, (uv_close_cb) wareq__destroy_cb);
}

int ti_wareq_unpack(ti_wareq_t * wareq, ti_pkg_t * pkg, ex_t * e)
{
    qp_unpacker_t unpacker;
    qp_obj_t key, val;
    const char * ebad = "invalid watch request, see "TI_DOCS"#watch";

    qp_unpacker_init2(&unpacker, pkg->data, pkg->n, 0);

    if (!qp_is_map(qp_next(&unpacker, NULL)))
    {
        ex_set(e, EX_BAD_DATA, ebad);
        goto finish;
    }
    while(qp_is_raw(qp_next(&unpacker, &key)))
    {
        if (qp_is_raw_equal_str(&key, "target"))
        {
            (void) qp_next(&unpacker, &val);
            wareq->target = ti_dbs_get_by_qp_obj(&val, e);
            if (wareq->target)
                ti_incref(wareq->target);
            if (e->nr)
                goto finish;
            continue;
        }

        if (qp_is_raw_equal_str(&key, "things"))
        {
            ssize_t n;
            qp_types_t tp = qp_next(&unpacker, NULL);

            if (!qp_is_array(tp))
            {
                ex_set(e, EX_BAD_DATA, ebad);
                goto finish;
            }

            n = tp == QP_ARRAY_OPEN ? -1 : (ssize_t) tp - QP_ARRAY0;

            wareq->thing_ids = vec_new(n < 0 ? 8 : n);
            if (!wareq->thing_ids)
            {
                ex_set_alloc(e);
                goto finish;
            }

            while(n-- && qp_is_int(qp_next(&unpacker, &val)))
            {
                uintptr_t id = (uintptr_t) val.via.int64;
                if (vec_push(&wareq->thing_ids, (void *) id))
                {
                    ex_set_alloc(e);
                    goto finish;
                }
            }
            continue;
        }

        if (!qp_is_map(qp_next(&unpacker, NULL)))
        {
            ex_set(e, EX_BAD_DATA, ebad);
            goto finish;
        }
    }

finish:
    return e->nr;
}

int ti_wareq_run(ti_wareq_t * wareq)
{
    return uv_async_send(wareq->task);
}

static void wareq__destroy(ti_wareq_t * wareq)
{
    vec_destroy(wareq->thing_ids, NULL);
    ti_stream_drop(wareq->stream);
    ti_db_drop(wareq->target);
    free(wareq->task);
    free(wareq);
}

static void wareq__destroy_cb(uv_async_t * task)
{
    ti_wareq_t * wareq = task->data;
    wareq__destroy(wareq);
}

static void wareq__task_cb(uv_async_t * task)
{
    ti_wareq_t * wareq = task->data;
    ti_thing_t * thing;
    uintptr_t id;
    uint32_t n = wareq->thing_ids->n;
    ti_watch_t * watch;

    if (!n || ti_stream_is_closed(wareq->stream))
    {
        ti_wareq_destroy(wareq);
        return;
    }

    uv_mutex_lock(wareq->target->lock);

    while (n--)
    {
        id = (uintptr_t) vec_pop(wareq->thing_ids);
        thing = imap_get(wareq->target->things, id);

        if (!thing)
            continue;

        watch = ti_thing_watch(thing, wareq->stream);
        if (!watch)
        {
            log_critical(EX_ALLOC_S);
            break;
        }

        if (ti_manages_id(id))
        {
            ti_pkg_t * pkg;
            qpx_packer_t * packer = qpx_packer_create(512, 4);
            if (!packer)
            {
                log_critical(EX_ALLOC_S);
                break;
            }

            (void) qp_add_map(&packer);
            (void) qp_add_raw_from_str(packer, "event");
            (void) qp_add_int64(packer, ti()->events->commit_event_id);
            (void) qp_add_raw_from_str(packer, "thing");

            if (    ti_thing_to_packer(thing, &packer, TI_VAL_PACK_ATTR) ||
                    qp_close_map(packer))
            {
                log_critical(EX_ALLOC_S);
                qp_packer_destroy(packer);
                break;
            }

            pkg = qpx_packer_pkg(packer, TI_PROTO_CLIENT_WATCH_INI);

            if (    ti_stream_is_closed(wareq->stream) ||
                    ti_clients_write(wareq->stream, pkg))
            {
                free(pkg);
            }

            continue;
        }
    }

    uv_mutex_unlock(wareq->target->lock);

    ti_wareq_destroy(wareq);
}

static int wareq__fwd_wareq(ti_wareq_t * wareq, uint64_t thing_id)
{
    qpx_packer_t * packer;
    ti_fwd_t * fwd;
    ti_pkg_t * pkg;
    ti_node_t * node;

    node = ti_nodes_random_ready_node_for_id(thing_id);
    if (!node)
    {
        log_critical("no online node found which manages "TI_THING_ID, thing_id);
        return -1;
    }

    packer = qpx_packer_create(9, 1);
    if (!packer)
        goto fail0;

    fwd = ti_fwd_create(0, wareq->stream);
    if (!fwd)
        goto fail1;

    (void) qp_add_int64(packer, thing_id);
    pkg = qpx_packer_pkg(packer, TI_PROTO_NODE_REQ_WATCH_ID);


    if (ti_req_create(
            node->stream,
            pkg,
            TI_PROTO_NODE_REQ_QUERY_TIMEOUT,
            clients__fwd_query_cb,
            fwd))
        goto fail1;

    return 0;

fail1:
    ti_fwd_destroy(fwd);
    qp_packer_destroy(packer);
fail0:
    log_critical(EX_ALLOC_S);
    return -1;
}

/*
 * TODO: This is (still) equal to fwd_query_cb ??? Then create a fwd function
 */
static void wareq__fwd_wareq_cb(ti_req_t * req, ex_enum status)
{
    ti_pkg_t * resp;
    ti_fwd_t * fwd = req->data;
    if (status)
    {
        ex_t * e = ex_use();
        ex_set(e, status, ex_str(status));
        resp = ti_pkg_err(fwd->orig_pkg_id, e);
        if (!resp)
            log_error(EX_ALLOC_S);
        goto finish;
    }

    resp = req->pkg_res;
    req->pkg_res = NULL;

    resp->id = fwd->orig_pkg_id;
    resp->tp = resp->tp - 0x80;
    resp->ntp = resp->tp ^ 255;

finish:
    free(req->pkg_res);
    if (resp && ti_clients_write(fwd->stream, resp))
    {
        free(resp);
        log_error(EX_ALLOC_S);
    }

    ti_fwd_destroy(fwd);
    ti_req_destroy(req);
}

