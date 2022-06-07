/*
 * ti/watch.h
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/proto.h>
#include <ti/watch.h>
#include <util/mpack.h>


ti_watch_t * ti_watch_create(ti_stream_t * stream)
{
    ti_watch_t * watch = malloc(sizeof(ti_watch_t));
    if (!watch)
        return NULL;
    watch->stream = stream;
    return watch;
}

