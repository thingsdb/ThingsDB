/*
 * ti/future.c
 */
#include <ti/future.h>
#include <ti/future.inline.h>
#include <ti/val.t.h>
#include <ti/val.inline.h>

ti_future_t * ti_future_create(
        ti_query_t * query,
        ti_module_t * module,
        size_t nargs,
        uint8_t deep)
{
    ti_future_t * future = malloc(sizeof(ti_future_t));
    if (!future)
        return NULL;

    future->ref = 1;
    future->tp = TI_VAL_FUTURE;
    future->deep = deep;
    future->query = query;
    future->rval = NULL;
    future->then = NULL;
    future->fail = NULL;
    future->pkg = NULL;
    future->module = module;
    future->args = vec_new(nargs);
    if (!future->args)
    {
        free(future);
        return NULL;
    }
    return future;
}

void ti_future_destroy(ti_future_t * future)
{
    if (!future)
        return;

    ti_future_forget_cb(future->then);
    ti_future_forget_cb(future->fail);
    vec_destroy(future->args, (vec_destroy_cb) ti_val_unsafe_drop);
    ti_val_drop(future->rval);
    free(future->pkg);
    free(future);
}

void ti_future_cancel(ti_future_t * future)
{
    ex_t e;  /* TODO: introduce new error ? */
    ex_set(e, EX_OPERATION_ERROR, "future cancelled before completion");
    ti_query_on_future_result(future, &e);
}
