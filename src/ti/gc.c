/*
 * ti/gc.c
 */
#include <stdlib.h>
#include <ti/gc.h>
#include <ti/val.inline.h>

ti_gc_t * ti_gc_create(uint64_t change_id, ti_thing_t * thing)
{
    ti_gc_t * gc = malloc(sizeof(ti_gc_t));
    if (!gc)
        return NULL;

    gc->change_id = change_id;
    gc->thing = thing;

    ti_incref(thing);

    return gc;
}

void ti_gc_destroy(ti_gc_t * gc)
{
    if (!gc)
        return;
    ti_val_unsafe_drop((ti_val_t *) gc->thing);
    free(gc);
}
