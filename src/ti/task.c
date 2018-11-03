/*
 * task.c
 */
#include <assert.h>
#include <qpack.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/task.h>
#include <util/qpx.h>

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

    unsigned char * subtask = qpx_get_and_destroy(packer);

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

