/*
 * util/res.c
 */
#include <ti/name.h>
#include <langdef/nd.h>
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

/* checks PROP, UNDEFINED, ARROW and ARRAY/TUPLE */
int res_assign_val(ti_res_t * res, _Bool to_array, ex_t * e)
{
    switch (res->rval->tp)
    {
    case TI_VAL_PROP:
        if (res_rval_clear(res))
            ex_set_alloc(e);
        else
            ti_val_set_nil(res->rval);
        break;
    case TI_VAL_UNDEFINED:
        ex_set(e, EX_BAD_DATA, "type `undefined` cannot be assigned");
        break;
    case TI_VAL_ARROW:
        if (ti_arrow_wse(res->rval->via.arrow))
            ex_set(e, EX_BAD_DATA,
                    "an arrow function with side effects cannot be assigned");
        break;
    case TI_VAL_TUPLE:
    case TI_VAL_ARRAY:
        res->rval->tp = to_array ? TI_VAL_TUPLE : TI_VAL_ARRAY;
        break;
    }
    return e->nr;
}







