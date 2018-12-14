/*
 * task.c
 */
#include <assert.h>
#include <qpack.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/task.h>
#include <ti/raw.h>
#include <ti/proto.h>
#include <util/qpx.h>
#include <util/cryptx.h>

static int task__thing_to_packer(qp_packer_t ** packer, ti_thing_t * thing);
static void task__upd_approx_sz(ti_task_t * task, ti_raw_t * raw);

ti_task_t * ti_task_create(uint64_t event_id, ti_thing_t * thing)
{
    ti_task_t * task = malloc(sizeof(ti_task_t));
    if (!task)
        return NULL;

    task->event_id = event_id;
    task->thing = ti_grab(thing);
    task->jobs = vec_new(1);
    task->approx_sz = 0;
    if (!task->jobs)
    {
        ti_task_destroy(task);
        return NULL;
    }

    return task;
}

ti_task_t * ti_task_get_task(ti_event_t * ev, ti_thing_t * thing, ex_t * e)
{
    ti_task_t * task = omap_get(ev->tasks, thing->id);
    if (task)
        return task;

    task = ti_task_create(ev->id, thing);
    if (!task)
        goto failed;

    if (omap_add(ev->tasks, thing->id, task))
        goto failed;

    return task;

failed:
    ti_task_destroy(task);
    ex_set_alloc(e);
    return NULL;
}

void ti_task_destroy(ti_task_t * task)
{
    if (!task)
        return;
    vec_destroy(task->jobs, free);
    ti_thing_drop(task->thing);
    free(task);
}

ti_pkg_t * ti_task_pkg_watch(ti_task_t * task)
{
    ti_pkg_t * pkg;
    qp_packer_t * packer = qpx_packer_create(task->approx_sz, 2);
    if (!packer)
        return NULL;
    (void) qp_add_map(&packer);
    (void) qp_add_raw(packer, (const uchar *) "#", 1);
    (void) qp_add_int64(packer, task->thing->id);
    (void) qp_add_raw_from_str(packer, "event");
    (void) qp_add_int64(packer, task->event_id);
    (void) qp_add_raw_from_str(packer, "jobs");
    (void) qp_add_array(&packer);
    for (vec_each(task->jobs, ti_raw_t, raw))
    {
        (void) qp_add_qp(packer, raw->data, raw->n);
    }
    (void) qp_close_array(packer);
    (void) qp_close_map(packer);

    pkg = qpx_packer_pkg(packer, TI_PROTO_CLIENT_WATCH_UPD);
    qp_print(pkg->data, pkg->n);

    return pkg;
}

int ti_task_add_assign(ti_task_t * task, ti_name_t * name, ti_val_t * val)
{
    int rc;
    ti_raw_t * job = NULL;
    qp_packer_t * packer = qp_packer_create2(512, 8);
    if (!packer)
        goto failed;

    if (ti_val_gen_ids(val))
        goto failed;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "assign");
    (void) qp_add_map(&packer);

    if (qp_add_raw(packer, (const uchar *) name->str, name->n))
        goto failed;

    if (ti_val_to_packer(val, &packer, TI_VAL_PACK_NEW))
        goto failed;

    if (qp_close_map(packer) || qp_close_map(packer))
        goto failed;

    job = ti_raw_from_packer(packer);
    if (!job)
        goto failed;

    if (vec_push(&task->jobs, job))
        goto failed;

    rc = 0;

    task__upd_approx_sz(task, job);
    goto done;

failed:
    ti_raw_drop(job);
    rc = -1;
done:
    if (packer)
        qp_packer_destroy(packer);
    return rc;
}

int ti_task_add_del(ti_task_t * task, ti_raw_t * rname)
{
    int rc;
    ti_raw_t * job = NULL;
    qp_packer_t * packer = qp_packer_create2(14 + rname->n, 1);
    if (!packer)
        goto failed;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "del");
    (void) qp_add_raw(packer, rname->data, rname->n);
    (void) qp_close_map(packer);

    job = ti_raw_from_packer(packer);
    if (!job)
        goto failed;

    if (vec_push(&task->jobs, job))
        goto failed;

    rc = 0;
    task__upd_approx_sz(task, job);
    goto done;

failed:
    ti_raw_drop(job);
    rc = -1;
done:
    if (packer)
        qp_packer_destroy(packer);
    return rc;
}

int ti_task_add_del_collection(ti_task_t * task, uint64_t collection_id)
{
    int rc;
    ti_raw_t * job = NULL;
    qp_packer_t * packer = qp_packer_create2(26, 1);
    if (!packer)
        goto failed;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "del_collection");
    (void) qp_add_int64(packer, collection_id);
    (void) qp_close_map(packer);

    job = ti_raw_from_packer(packer);
    if (!job)
        goto failed;

    if (vec_push(&task->jobs, job))
        goto failed;

    rc = 0;
    task__upd_approx_sz(task, job);
    goto done;

failed:
    ti_raw_drop(job);
    rc = -1;
done:
    if (packer)
        qp_packer_destroy(packer);
    return rc;
}

int ti_task_add_del_user(ti_task_t * task, ti_user_t * user)
{
    int rc;
    ti_raw_t * job = NULL;
    qp_packer_t * packer = qp_packer_create2(20, 1);
    if (!packer)
        goto failed;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "del_user");
    (void) qp_add_int64(packer, user->id);
    (void) qp_close_map(packer);

    job = ti_raw_from_packer(packer);
    if (!job)
        goto failed;

    if (vec_push(&task->jobs, job))
        goto failed;

    rc = 0;
    task__upd_approx_sz(task, job);
    goto done;

failed:
    ti_raw_drop(job);
    rc = -1;
done:
    if (packer)
        qp_packer_destroy(packer);
    return rc;
}

int ti_task_add_grant(
        ti_task_t * task,
        uint64_t target_id,
        ti_user_t * user,
        uint64_t mask)
{
    int rc;
    ti_raw_t * job = NULL;
    qp_packer_t * packer = qp_packer_create2(50 + user->name->n, 2);
    if (!packer)
        goto failed;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "grant");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "target");
    (void) qp_add_int64(packer, target_id);
    (void) qp_add_raw_from_str(packer, "user");
    (void) qp_add_int64(packer, user->id);
    (void) qp_add_raw_from_str(packer, "mask");
    (void) qp_add_int64(packer, mask);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    job = ti_raw_from_packer(packer);
    if (!job)
        goto failed;

    if (vec_push(&task->jobs, job))
        goto failed;

    rc = 0;
    task__upd_approx_sz(task, job);
    goto done;

failed:
    ti_raw_drop(job);
    rc = -1;
done:
    if (packer)
        qp_packer_destroy(packer);
    return rc;
}

int ti_task_add_new_collection(
        ti_task_t * task,
        ti_collection_t * collection,
        ti_user_t * user)
{
    int rc;
    ti_raw_t * job = NULL;
    qp_packer_t * packer = qp_packer_create2(
            40 + collection->name->n + user->name->n, 2);

    if (!packer)
        goto failed;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "new_collection");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "name");
    (void) qp_add_raw(packer, collection->name->data, collection->name->n);
    (void) qp_add_raw_from_str(packer, "user");
    (void) qp_add_int64(packer, user->id);
    (void) qp_add_raw_from_str(packer, "root");
    (void) qp_add_int64(packer, collection->root->id);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    job = ti_raw_from_packer(packer);
    if (!job)
        goto failed;

    if (vec_push(&task->jobs, job))
        goto failed;

    rc = 0;
    task__upd_approx_sz(task, job);
    goto done;

failed:
    ti_raw_drop(job);
    rc = -1;
done:
    if (packer)
        qp_packer_destroy(packer);
    return rc;
}

int ti_task_add_new_node(ti_task_t * task, ti_node_t * node)
{
    int rc;
    ti_raw_t * job = NULL;
    qp_packer_t * packer = qp_packer_create2(256, 2);

    if (!packer)
        goto failed;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "new_node");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "id");
    (void) qp_add_int64(packer, node->id);
    (void) qp_add_raw_from_str(packer, "zone");
    (void) qp_add_int64(packer, node->zone);
    (void) qp_add_raw_from_str(packer, "port");
    (void) qp_add_int64(packer, node->port);
    (void) qp_add_raw_from_str(packer, "addr");
    (void) qp_add_raw_from_str(packer, node->addr);
    (void) qp_add_raw_from_str(packer, "secret");
    (void) qp_add_raw(packer, (uchar *) node->secret, CRYPTX_SZ);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    job = ti_raw_from_packer(packer);
    if (!job)
        goto failed;

    if (vec_push(&task->jobs, job))
        goto failed;

    rc = 0;
    task__upd_approx_sz(task, job);
    goto done;

failed:
    ti_raw_drop(job);
    rc = -1;
done:
    if (packer)
        qp_packer_destroy(packer);
    return rc;
}

int ti_task_add_new_user(ti_task_t * task, ti_user_t * user)
{
    int rc;
    ti_raw_t * job = NULL;
    qp_packer_t * packer = qp_packer_create2(40 + user->name->n, 2);

    if (!packer)
        goto failed;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "new_user");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "id");
    (void) qp_add_int64(packer, user->id);
    (void) qp_add_raw_from_str(packer, "username");
    (void) qp_add_raw(packer, user->name->data, user->name->n);
    (void) qp_add_raw_from_str(packer, "password");
    (void) qp_add_raw_from_str(packer, user->encpass);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    job = ti_raw_from_packer(packer);
    if (!job)
        goto failed;

    if (vec_push(&task->jobs, job))
        goto failed;

    rc = 0;
    task__upd_approx_sz(task, job);
    goto done;

failed:
    ti_raw_drop(job);
    rc = -1;
done:
    if (packer)
        qp_packer_destroy(packer);
    return rc;
}

int ti_task_add_pop_node(ti_task_t * task, uint8_t node_id)
{
    int rc;
    ti_raw_t * job = NULL;
    qp_packer_t * packer = qp_packer_create2(16, 1);

    if (!packer)
        goto failed;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "pop_node");
    (void) qp_add_int64(packer, node_id);
    (void) qp_close_map(packer);

    job = ti_raw_from_packer(packer);
    if (!job)
        goto failed;

    if (vec_push(&task->jobs, job))
        goto failed;

    rc = 0;
    task__upd_approx_sz(task, job);
    goto done;

failed:
    ti_raw_drop(job);
    rc = -1;
done:
    if (packer)
        qp_packer_destroy(packer);
    return rc;
}

int ti_task_add_push(
        ti_task_t * task,
        ti_name_t * name,
        ti_val_t * val,             /* array or things */
        size_t n)
{
    assert (val->tp == TI_VAL_THINGS || val->tp == TI_VAL_ARRAY);
    assert (val->via.arr->n >= n);
    assert (name);
    int rc;
    ti_raw_t * job = NULL;
    size_t m = val->via.arr->n;
    qp_packer_t * packer = qp_packer_create2(512, 8);
    if (!packer)
        goto failed;

    if (ti_val_gen_ids(val))
        goto failed;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "push");
    (void) qp_add_map(&packer);

    if (qp_add_raw(packer, (const uchar *) name->str, name->n) ||
        qp_add_array(&packer))
        goto failed;

    if (val->tp == TI_VAL_THINGS)
    {
        for (n = m - n; n < m; ++n)
        {
            ti_thing_t * t = vec_get(val->via.things, n);
            if (task__thing_to_packer(&packer, t))
                goto failed;
        }
    }
    else
    {
        assert (val->tp == TI_VAL_ARRAY);
        for (n = m - n; n < m; ++n)
        {
            ti_val_t * v = vec_get(val->via.array, n);
            if (ti_val_to_packer(v, &packer, 0))
                goto failed;
        }
    }

    if (qp_close_array(packer) || qp_close_map(packer) || qp_close_map(packer))
        goto failed;

    job = ti_raw_from_packer(packer);
    if (!job)
        goto failed;

    if (vec_push(&task->jobs, job))
        goto failed;

    rc = 0;
    task__upd_approx_sz(task, job);
    goto done;

failed:
    ti_raw_drop(job);
    rc = -1;
done:
    if (packer)
        qp_packer_destroy(packer);
    return rc;
}

int ti_task_add_rename(ti_task_t * task, ti_raw_t * from, ti_raw_t * to)
{
    int rc;
    ti_raw_t * job = NULL;
    qp_packer_t * packer = qp_packer_create2(30 + from->n + to->n, 2);
    if (!packer)
        goto failed;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "rename");
    (void) qp_add_map(&packer);
    (void) qp_add_raw(packer, from->data, from->n);
    (void) qp_add_raw(packer, to->data, to->n);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    job = ti_raw_from_packer(packer);
    if (!job)
        goto failed;

    if (vec_push(&task->jobs, job))
        goto failed;

    rc = 0;
    task__upd_approx_sz(task, job);
    goto done;

failed:
    ti_raw_drop(job);
    rc = -1;
done:
    if (packer)
        qp_packer_destroy(packer);
    return rc;
}

int ti_task_add_revoke(
        ti_task_t * task,
        uint64_t target_id,
        ti_user_t * user,
        uint64_t mask)
{
    int rc;
    ti_raw_t * job = NULL;
    qp_packer_t * packer = qp_packer_create2(50 + user->name->n, 2);
    if (!packer)
        goto failed;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "revoke");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "target");
    (void) qp_add_int64(packer, target_id);
    (void) qp_add_raw_from_str(packer, "user");
    (void) qp_add_int64(packer, user->id);
    (void) qp_add_raw_from_str(packer, "mask");
    (void) qp_add_int64(packer, mask);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    job = ti_raw_from_packer(packer);
    if (!job)
        goto failed;

    if (vec_push(&task->jobs, job))
        goto failed;

    rc = 0;
    task__upd_approx_sz(task, job);
    goto done;

failed:
    ti_raw_drop(job);
    rc = -1;
done:
    if (packer)
        qp_packer_destroy(packer);
    return rc;
}

int ti_task_add_set(ti_task_t * task, ti_name_t * name, ti_val_t * val)
{
    int rc;
    ti_raw_t * job = NULL;
    qp_packer_t * packer = qp_packer_create2(512, 8);
    if (!packer)
        goto failed;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "set");
    (void) qp_add_map(&packer);

    if (qp_add_raw(packer, (const uchar *) name->str, name->n))
        goto failed;

    if (ti_val_to_packer(val, &packer, TI_VAL_PACK_NEW))
        goto failed;

    if (qp_close_map(packer) || qp_close_map(packer))
        goto failed;

    job = ti_raw_from_packer(packer);
    if (!job)
        goto failed;

    if (vec_push(&task->jobs, job))
        goto failed;

    rc = 0;
    task__upd_approx_sz(task, job);
    goto done;

failed:
    ti_raw_drop(job);
    rc = -1;
done:
    if (packer)
        qp_packer_destroy(packer);
    return rc;
}

int ti_task_add_set_quota(
        ti_task_t * task,
        uint64_t collection_id,
        ti_quota_enum_t quota_tp,
        size_t quota)
{
    int rc;
    ti_raw_t * job = NULL;
    qp_packer_t * packer = qp_packer_create2(128, 2);
    if (!packer)
        goto failed;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "set_quota");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "collection");
    (void) qp_add_int64(packer, collection_id);
    (void) qp_add_raw_from_str(packer, "quota_tp");
    (void) qp_add_int64(packer, quota_tp);
    (void) qp_add_raw_from_str(packer, "quota");
    (void) qp_add_int64(packer, quota);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    job = ti_raw_from_packer(packer);
    if (!job)
        goto failed;

    if (vec_push(&task->jobs, job))
        goto failed;

    rc = 0;
    task__upd_approx_sz(task, job);
    goto done;

failed:
    ti_raw_drop(job);
    rc = -1;
done:
    if (packer)
        qp_packer_destroy(packer);
    return rc;
}

int ti_task_add_splice(
        ti_task_t * task,
        ti_name_t * name,
        ti_val_t * val,
        int64_t i,
        int64_t c,
        int32_t n)
{
    assert (val->tp == TI_VAL_THINGS || val->tp == TI_VAL_ARRAY);
    assert (name);
    int rc;
    ti_raw_t * job = NULL;
    qp_packer_t * packer = qp_packer_create2(512, 8);
    if (!packer)
        goto failed;

    if (ti_val_gen_ids(val))
        goto failed;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "splice");
    (void) qp_add_map(&packer);

    if (qp_add_raw(packer, (const uchar *) name->str, name->n) ||
        qp_add_array(&packer) ||
        qp_add_int64(packer, i) ||
        qp_add_int64(packer, c) ||
        qp_add_int64(packer, n))
        goto failed;

    if (val->tp == TI_VAL_THINGS)
    {
        for (c = i + n; i < c; ++i)
        {
            ti_thing_t * t = vec_get(val->via.things, i);
            if (task__thing_to_packer(&packer, t))
                goto failed;
        }
    }
    else
    {
        assert (val->tp == TI_VAL_ARRAY);
        for (c = i + n; i < c; ++i)
        {
            ti_val_t * v = vec_get(val->via.array, i);
            if (ti_val_to_packer(v, &packer, 0))
                goto failed;
        }
    }

    if (qp_close_array(packer) || qp_close_map(packer) || qp_close_map(packer))
        goto failed;

    job = ti_raw_from_packer(packer);
    if (!job)
        goto failed;

    if (vec_push(&task->jobs, job))
        goto failed;

    rc = 0;
    task__upd_approx_sz(task, job);
    goto done;

failed:
    ti_raw_drop(job);
    rc = -1;
done:
    if (packer)
        qp_packer_destroy(packer);
    return rc;
}

int ti_task_add_unset(ti_task_t * task, ti_name_t * name)
{
    int rc;
    ti_raw_t * job = NULL;
    qp_packer_t * packer = qp_packer_create2(16 + name->n, 8);
    if (!packer)
        goto failed;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "unset");
    (void) qp_add_raw(packer, (const uchar *) name->str, name->n);
    (void) qp_close_map(packer);

    job = ti_raw_from_packer(packer);
    if (!job)
        goto failed;

    if (vec_push(&task->jobs, job))
        goto failed;

    rc = 0;
    task__upd_approx_sz(task, job);
    goto done;

failed:
    ti_raw_drop(job);
    rc = -1;
done:
    if (packer)
        qp_packer_destroy(packer);
    return rc;
}

static int task__thing_to_packer(qp_packer_t ** packer, ti_thing_t * thing)
{
    if (thing->flags & TI_THING_FLAG_NEW)
        return ti_thing_to_packer(thing, packer, TI_VAL_PACK_NEW);
    return ti_thing_id_to_packer(thing, packer);
}

static inline void task__upd_approx_sz(ti_task_t * task, ti_raw_t * raw)
{
    task->approx_sz += (37 + raw->n);
}

