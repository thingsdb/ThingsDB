/*
 * ti/modasync.c
 */
#include <ti/async.h>
#include <ti/varr.h>
#include <ti/val.inline.h>
#include <ti/query.h>
#include <ti.h>

static void async__uv_cb(uv_async_t * task)
{
    ex_t e = {0};
    ti_future_t * future = task->data;

    /*
     * We never "use" the array directly, only the members in combination with
     * a `then` or `else` case.
     * For these `then` and `else` cases we do not want to convert
     * lists to tuples, therefore we should make this a "special"
     * array where items are explicitly *not* converted.
     */
    future->rval = (ti_val_t *) ti_varr_from_vec_unsafe(future->args);
    if (future->rval)
        future->args = NULL;
    else
        ex_set_mem(&e);

    ti_query_on_future_result(future, &e);
    uv_close((uv_handle_t *) task, (uv_close_cb) free);
}

static void async__cb(ti_future_t * future)
{
    uv_async_t * task = malloc(sizeof(uv_async_t));
    if (!task)
    {
        ex_t e;
        ex_set_mem(&e);
        ti_query_on_future_result(future, &e);
        return;
    }

    task->data = future;

    if (uv_async_init(ti.loop, task, (uv_async_cb) async__uv_cb) ||
        uv_async_send(task))
    {
        ex_t e;
        ex_set_internal(&e);
        ti_query_on_future_result(future, &e);
    }
}

static ti_module_t async__module = {
        .ref = 1,
        .status = TI_MODULE_STAT_RUNNING,
        .cb = (ti_module_cb) async__cb,
        .name = NULL,
};

void ti_async_init(void)
{
    async__module.name = (ti_name_t *) ti_val_borrow_async_name();
}

ti_module_t * ti_async_get_module(void)
{
    return &async__module;
}
