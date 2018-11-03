/*
 * util/res.c
 */
#include <ti/name.h>
#include <util/imap.h>
#include <util/res.h>


void res_destroy_collect_cb(vec_t * names)
{
    vec_destroy(names, (vec_destroy_cb) ti_name_drop);
}

ti_task_t * res_get_task(ti_event_t * ev, ti_thing_t * thing, ex_t * e)
{
    ti_task_t * task = imap_get(ev->tasks, thing->id);
    if (task)
        return task;

    task = ti_task_create(ev->id, thing);
    if (!task)
        goto failed;

    if (imap_add(ev->tasks, thing->id, task))
        goto failed;

    return task;

failed:
    ti_task_destroy(task);
    ex_set_alloc(e);
    return NULL;
}


