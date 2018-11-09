/*
 * util/res.c
 */
#include <ti/name.h>
#include <util/omap.h>
#include <util/res.h>


void res_destroy_collect_cb(vec_t * names)
{
    vec_destroy(names, (vec_destroy_cb) ti_name_drop);
}

ti_task_t * res_get_task(ti_event_t * ev, ti_thing_t * thing, ex_t * e)
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

int res_rval_clear(ti_res_t * res)
{
    if (res->rval)
    {
        ti_val_clear(res->rval);
        return 0;
    }
    res->rval = ti_val_create(TI_VAL_UNDEFINED, NULL);
    return res->rval ? 0 : -1;
}

void res_rval_destroy(ti_res_t * res)
{
    ti_val_destroy(res->rval);
    res->rval = NULL;
}

void res_rval_weak_destroy(ti_res_t * res)
{
    ti_val_weak_destroy(res->rval);
    res->rval = NULL;
}



