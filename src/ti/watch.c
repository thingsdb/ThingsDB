/*
 * ti/watch.h
 */
#include <ti/watch.h>
#include <ti.h>
#include <assert.h>
#include <stdlib.h>


ti_watch_t * ti_watch_create(ti_stream_t * stream)
{
    ti_watch_t * watch = malloc(sizeof(ti_watch_t));
    if (!watch)
        return NULL;
    watch->stream = stream;
    return watch;
}

void ti_watch_drop(ti_watch_t * watch)
{
    if (!watch)
        return;
    if (watch->stream)
        watch->stream = NULL;
    else
        free(watch);
}
