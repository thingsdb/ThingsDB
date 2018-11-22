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





static void watch__init(ti_db_t * db, vec_t * thing_ids)
{
    int rc = -1;
    uv_mutex_lock(db->lock);

    for (uint32_t i = 0; i < thing_ids->n; ++i)
    {
        qpx_packer_t * packer = qpx_packer_create(512, 4);

        uintptr_t id = (uintptr_t) vec_get(thing_ids, i);
        ti_thing_t * thing = imap_get(db->things, id);

    }


done:
    uv_mutex_unlock(db->lock);
    return rc;
}
