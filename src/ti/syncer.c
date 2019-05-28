/*
 * ti/syncer.c
 */
#include <assert.h>
#include <ti/syncer.h>
#include <ti.h>


ti_syncer_t * ti_syncer_create(ti_stream_t * stream, uint64_t first)
{
    ti_syncer_t * syncer = malloc(sizeof(ti_syncer_t));
    if (!syncer)
        return NULL;

    syncer->stream = stream;
    syncer->first = first;

    return syncer;
}
