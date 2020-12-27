/*
 * ti/future.c
 */
#include <ti/future.h>
#include <ti/val.t.h>
#include <ti/val.inline.h>

ti_future_t * ti_future_create(
        ti_query_t * query,
        ti_ext_t * ext,
        size_t nargs)
{
    ti_future_t * future = malloc(sizeof(ti_future_t));
    if (!future)
        return NULL;

    future->ref = 1;
    future->tp = TI_VAL_FUTURE;
    future->query = query;
    future->rval = NULL;
    future->then = NULL;
    future->ext = ext;
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
    /*
     * TODO: check if GC drops are required here.
     */
    vec_destroy(future->args, (vec_destroy_cb) ti_val_unsafe_gc_drop);
    ti_val_drop((ti_val_t *) future->then);
    ti_val_gc_drop(future->rval);
    free(future);
}

