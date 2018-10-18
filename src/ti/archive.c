/*
 * archive.c
 */
#include <stdlib.h>
#include <ti/archive.h>
#include <util/logger.h>

ti_archive_t * ti_archive_create(void)
{
    ti_archive_t * archive = malloc(sizeof(ti_archive_t));
    if (!archive)
        return NULL;

    archive->rawev = queue_new(0);
    if (!archive->rawev)
    {
        ti_archive_destroy(archive);
        return NULL;
    }

    return archive;
}

void ti_archive_destroy(ti_archive_t * archive)
{
    if (!archive)
        return;
    queue_destroy(archive->rawev, free);
    free(archive);
}

int ti_archive_event(ti_archive_t * archive, ti_event_t * event)
{
    if (!archive->rawev->n)
    {
        archive->offset = event->id;
    }
    if (queue_push(&archive->rawev, (event->status == TI_EVENT_STAT_CACNCEL) ?
            NULL : event->raw))
    {
        log_critical("failed to archive event id: %"PRIu64, event->id);
        return -1;
    }
    event->raw = NULL;
    return 0;
}
