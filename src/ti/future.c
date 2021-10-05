/*
 * ti/future.c
 */
#include <ti/async.h>
#include <ti/future.h>
#include <ti/future.inline.h>
#include <ti/val.t.h>
#include <ti/val.inline.h>
#include <ti.h>

static inline int future__reg(ti_future_t * future)
{
    ti_collection_t * collection = future->query->collection;
    return collection ? vec_push(&collection->futures, future) : 0;
}

static void future__unreg(ti_future_t * future)
{
    ti_collection_t * collection = future->query->collection;
    if (collection)
    {
        size_t idx = 0;
        for (vec_each(collection->futures, ti_future_t, f), ++idx)
        {
            if (future == f)
            {
                (void) vec_swap_remove(collection->futures, idx);
                return;
            }
        }
    }
}

ti_future_t * ti_future_create(
        ti_query_t * query,
        ti_module_t * module,
        size_t nargs,
        uint8_t deep,
        _Bool load)
{
    ti_future_t * future = malloc(sizeof(ti_future_t));
    if (!future)
        return NULL;

    future->ref = 1;
    future->tp = TI_VAL_FUTURE;
    future->options = deep;
    if (load)
        future->options |= TI_FUTURE_LOAD_FLAG;
    future->query = query;
    future->rval = NULL;
    future->then = NULL;
    future->fail = NULL;
    future->pkg = NULL;
    future->module = module;
    if (nargs)
    {
        future->args = vec_new(nargs);
        if (!future->args)
        {
            ti_future_destroy(future);
            return NULL;
        }
    }
    else
        future->args = NULL;  /* Future arguments may stay NULL for as long as
                                 the future is not registered. A registered
                                 future MUST have a vector with arguments. */

    ti_incref(module);
    return future;
}

void ti_future_destroy(ti_future_t * future)
{
    if (!future)
        return;
    future__unreg(future);
    ti_future_forget_cb(future->then);
    ti_future_forget_cb(future->fail);
    vec_destroy(future->args, (vec_destroy_cb) ti_val_unsafe_drop);
    ti_val_drop(future->rval);
    ti_module_drop(future->module);
    free(future->pkg);
    free(future);
}

void ti_future_cancel(ti_future_t * future)
{
    ex_t e;
    ex_set(&e, EX_CANCELLED, "future cancelled before completion");
    ti_query_on_future_result(future, &e);
}

void ti_future_stop(ti_future_t * future)
{
    if (future->module != ti_async_get_module())
        (void) omap_rm(future->module->futures, future->pid);

    ti_future_cancel(future);
}
