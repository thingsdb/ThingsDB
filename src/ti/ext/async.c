/*
 * ti/ext/async.c
 */
#include <ti/ext/async.h>
#include <ti/varr.h>
#include <ti/query.h>
#include <ti.h>


static ti_ext_t ext_async = {
        .cb = ti_ext_async_cb,
        .destroy_cb = NULL,
        .data = NULL,
};

static void ext_async__cb(uv_async_t * task)
{
    ex_t e = {0};
    ti_future_t * future = task->data;

    future->rval = (ti_val_t *) ti_varr_from_vec(future->args);
    if (future->rval)
        future->args = NULL;
    else
        ex_set_mem(&e);

    ti_query_on_future_result(future, &e);
    uv_close((uv_handle_t *) task, (uv_close_cb) free);
}

ti_ext_t * ti_ext_async_get(void)
{
    return &ext_async;
}

void ti_ext_async_cb(ti_future_t * future, void * UNUSED(data))
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

    if (uv_async_init(ti.loop, task, (uv_async_cb) ext_async__cb) ||
        uv_async_send(task))
    {
        ex_t e;
        ex_set_internal(&e);
        ti_query_on_future_result(future, &e);
    }
}

