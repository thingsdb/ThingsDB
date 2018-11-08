/*
 * task.c
 */
#include <assert.h>
#include <qpack.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/task.h>
#include <util/qpx.h>

static int task__thing_to_packer(qp_packer_t ** packer, ti_thing_t * thing);

ti_task_t * ti_task_create(uint64_t event_id, ti_thing_t * thing)
{
    ti_task_t * task = malloc(sizeof(ti_task_t));
    if (!task)
        return NULL;

    task->event_id = event_id;
    task->thing = ti_grab(thing);
    task->subtasks = vec_new(1);
    if (!task->subtasks)
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
    vec_destroy(task->subtasks, free);
    ti_thing_drop(task->thing);
    free(task);
}

int ti_task_add_assign(ti_task_t * task, ti_name_t * name, ti_val_t * val)
{
    unsigned char * subtask;
    qp_packer_t * packer = qp_packer_create2(512, 8);
    if (!packer)
        goto failed;

    if (ti_val_gen_ids(val))
        goto failed;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "assign");
    (void) qp_add_map(&packer);

    if (qp_add_raw(packer, (const unsigned char *) name->str, name->n))
        goto failed;

    if (ti_val_to_packer(val, &packer, TI_VAL_PACK_NEW))
        goto failed;

    if (qp_close_map(packer) || qp_close_map(packer))
        goto failed;

    subtask = qpx_get_and_destroy(packer);

    if (vec_push(&task->subtasks, subtask))
    {
        free(subtask);
        return -1;
    }

    return 0;

failed:
    if (packer)
        qp_packer_destroy(packer);
    return -1;
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

    unsigned char * subtask;
    size_t m = val->via.arr->n;
    qp_packer_t * packer = qp_packer_create2(512, 8);
    if (!packer)
        goto failed;

    if (ti_val_gen_ids(val))
        goto failed;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "push");
    (void) qp_add_map(&packer);

    if (qp_add_raw(packer, (const unsigned char *) name->str, name->n) ||
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

    subtask = qpx_get_and_destroy(packer);

    if (vec_push(&task->subtasks, subtask))
    {
        free(subtask);
        return -1;
    }

    return 0;

failed:
    if (packer)
        qp_packer_destroy(packer);
    return -1;
}


static int task__thing_to_packer(qp_packer_t ** packer, ti_thing_t * thing)
{
    if (thing->flags & TI_THING_FLAG_NEW)
        return ti_thing_to_packer(thing, packer, TI_VAL_PACK_NEW);
    return ti_thing_id_to_packer(thing, packer);
}
