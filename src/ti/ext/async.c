/*
 * ti/ext/async.c
 */
#include <ti/ext/async.h>
#include <ti/varr.h>
#include <ti/query.h>
#include <ti.h>

static ti_ext_async_t ext_async;


ti_ext_t * ti_ext_async(void)
{
    return (ti_ext_t *) &ext_async;
}

static void ext_async__cb(uv_async_t * task)
{
    ti_future_t * future = task->data;
    future->rval = (ti_val_t *) ti_varr_from_vec(future->args);
    ti_query_on_future_result(future, future->rval ? 0 : EX_MEMORY);
    uv_close(task, free);
}

void ti_ext_async_cb(ti_future_t * future)
{
    uv_async_t * task = malloc(sizeof(uv_async_t));
    if (!task)
        goto fail;

    task->data = future;

    if (uv_async_init(ti.loop, task, (uv_async_cb) ext_async__cb) ||
        uv_async_send(task))
        ti_query_on_future_result(future, EX_INTERNAL);

    return;
fail:
    ti_query_on_future_result(future, EX_MEMORY);
}

