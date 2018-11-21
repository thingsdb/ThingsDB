/*
 * ti/watch.h
 */
#include <ti/watch.h>
#include <ti.h>
#include <assert.h>
#include <stdlib.h>

ti_watch_t * ti_watch_create(uint64_t id, ti_stream_t * stream)
{
    ti_watch_t * watchers, * watch = malloc(sizeof(ti_watch_t));
    if (!watch)
        return NULL;

    watch->stream = stream;
    watchers = imap_get(ti()->watchers, id);
    if (watchers)
    {
        ti_watch_t * tmp = watchers->next;
        watchers->next = watch;
        watch->prev = watchers;
        watch->next = tmp;
        if (tmp)
            tmp->prev = watch;
        return watch;
    }

    if (imap_add(ti()->watchers, id, watch))
    {
        free(watch);
        return NULL;
    }
    watch->prev = NULL;
    watch->next = NULL;
    return watch;
}

void ti_watch_destroy(ti_watch_t * watch)
{
    if (watch->)
}
