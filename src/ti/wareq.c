/*
 * ti/wareq.c
 */
#include <ti/wareq.h>
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/dbs.h>
#include <ti/proto.h>
#include <util/qpx.h>

static void wareq__destroy(ti_wareq_t * wareq);
static void wareq__destroy_cb(uv_async_t * task);
static void wareq__task_cb(uv_async_t * task);


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

        if (    ti_thing_to_packer(thing, &packer, TI_VAL_PACK_FETCH) ||
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

        break;
    }

    uv_mutex_unlock(wareq->target->lock);

    if (uv_async_send(wareq->task))
    {
        log_critical(EX_INTERNAL_S);
        ti_wareq_destroy(wareq);
    }
}

