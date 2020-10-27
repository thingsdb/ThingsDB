/*
 * ti/gc.c
 */
#include <stdlib.h>
#include <ti/gc.h>

ti_gc_t * ti_gc_create(uint64_t event_id, ti_thing_t * thing)
{
    ti_gc_t * gc = malloc(sizeof(ti_gc_t));
    if (!gc)
        return NULL;

    gc->event_id = event_id;
    gc->thing = thing;

    return gc;
}

