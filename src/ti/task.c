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

static int task__thing_to_packer(qp_packer_t ** packer, ti_thing_t * thing);
static size_t task__approx_watch_size(ti_task_t * task);

ti_task_t * ti_task_create(uint64_t event_id, ti_thing_t * thing)
{
    ti_task_t * task = malloc(sizeof(ti_task_t));
    if (!task)
        return NULL;

    task->event_id = event_id;
    task->thing = ti_grab(thing);
    task->jobs = vec_new(1);
    if (!task->jobs)
    {
        ti_task_destroy(task);
        return NULL;
    }

    return task;
}

void ti_task_destroy(ti_task_t * task)
{
    if (!task)
        return;
    vec_destroy(task->jobs, free);
    ti_thing_drop(task->thing);
    free(task);
}

/*
 *
    {
        '#': 4,
        'event': 0,
        'jobs': [
            {'assign': {<prop name>, <value>}},
            {'del': <prop name>},
            {'set': {<attr name>: <value>}},
            {'unset': <attr name>},
            {'push': {<prop name>: [{'#': 123}]}}
        ]
    }
 */
ti_pkg_t * ti_task_watch(ti_task_t * task)
{
    ti_pkg_t * pkg;
    size_t approx_size = task__approx_watch_size(task);
    qp_packer_t * packer = qpx_packer_create(approx_size, 2);
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
    goto done;

failed:
    ti_raw_drop(job);
    rc = -1;
done:
    if (packer)
        qp_packer_destroy(packer);
    return rc;
}

int ti_task_add_del(ti_task_t * task, ti_name_t * name)
{
    int rc;
    ti_raw_t * job = NULL;
    qp_packer_t * packer = qp_packer_create2(14 + name->n, 8);
    if (!packer)
        goto failed;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "del");
    (void) qp_add_raw(packer, (const uchar *) name->str, name->n);
    (void) qp_close_map(packer);

    job = ti_raw_from_packer(packer);
    if (!job)
        goto failed;

    if (vec_push(&task->jobs, job))
        goto failed;

    rc = 0;
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
    goto done;

failed:
    ti_raw_drop(job);
    rc = -1;
done:
    if (packer)
        qp_packer_destroy(packer);
    return rc;
}

int ti_task_add_new_database(ti_task_t * task, ti_db_t * db, ti_user_t * user)
{
    int rc;
    ti_raw_t * job = NULL;
    qp_packer_t * packer = qp_packer_create2(
            128 + db->name->n + user->name->n, 4);

    if (!packer)
        goto failed;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "new");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "databases");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "name");
    (void) qp_add_raw(packer, db->name->data, db->name->n);
    (void) qp_add_raw_from_str(packer, "user");
    (void) qp_add_raw(packer, user->name->data, user->name->n);
    (void) qp_add_raw_from_str(packer, "root");
    (void) ti_thing_id_to_packer(ti()->thing0, &packer);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    job = ti_raw_from_packer(packer);
    if (!job)
        goto failed;

    if (vec_push(&task->jobs, job))
        goto failed;

    rc = 0;
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

static size_t task__approx_watch_size(ti_task_t * task)
{
    size_t sz = 37;  /* maximum overhead */
    for (vec_each(task->jobs, ti_raw_t, raw))
        sz += raw->n;
    return sz;
}

