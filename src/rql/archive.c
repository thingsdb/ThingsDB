/*
 * archive.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <rql/archive.h>
#include <util/logger.h>

rql_archive_t * rql_archive_create(void)
{
    rql_archive_t * archive = malloc(sizeof(rql_archive_t));
    if (!archive) return NULL;

    archive->rawev = queue_new(0);
    if (!archive->rawev)
    {
        rql_archive_destroy(archive);
        return NULL;
    }

    return archive;
}

void rql_archive_destroy(rql_archive_t * archive)
{
    if (!archive) return;
    queue_destroy(archive->rawev, free);
    free(archive);
}

int rql_archive_event(rql_archive_t * archive, rql_event_t * event)
{
    if (!archive->rawev->n)
    {
        archive->offset = event->id;
    }
    if (queue_push(&archive->rawev, (event->status == RQL_EVENT_STAT_CACNCEL) ?
            NULL : event->raw))
    {
        log_critical("failed to archive event id: %"PRIu64, event->id);
        return -1;
    }
    event->raw = NULL;
    return 0;
}
