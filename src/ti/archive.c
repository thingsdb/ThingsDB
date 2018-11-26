/*
 * ti/archive.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/epkg.h>
#include <ti.h>
#include <util/logger.h>
#include <util/fx.h>
#include <util/util.h>
#include <unistd.h>

static const char * archive__path          = ".archive/";

static ti_archive_t * archive;


int ti_archive_create(void)
{
    archive = malloc(sizeof(ti_archive_t));
    if (!archive)
        return -1;

    archive->queue = queue_new(0);
    archive->path = NULL;
    archive->last_on_disk = 0;

    if (!archive->queue)
    {
        ti_archive_destroy();
        return -1;
    }

    ti()->archive = archive;
    return 0;
}

void ti_archive_destroy(void)
{
    if (!archive)
        return;
    queue_destroy(archive->queue, (queue_destroy_cb) ti_epkg_drop);
    free(archive->path);
    free(archive);
    archive = ti()->archive = NULL;
}

int ti_archive_load(void)
{
    char * storage_path = ti()->cfg->storage_path;
    assert (storage_path);
    assert (archive->path == NULL);

    archive->path = fx_path_join(storage_path, archive__path);
    if (!archive->path)
        return -1;

    if (!fx_is_dir(archive->path) && mkdir(archive->path, 0700))
    {
        log_critical("cannot create archive directory: `%s`", archive->path);
        ti_archive_destroy();
        return -1;
    }

    return 0;
}

int ti_archive_push(ti_epkg_t * epkg)
{
    return queue_push(&archive->queue, epkg);
}

int ti_archive_to_disk(_Bool with_sleep)
{
    FILE * f;
    char * fn;
    char buf[24];
    ti_epkg_t * last_epkg = queue_last(archive->queue);
    if (!last_epkg)
        return 0;       /* nothing to save to disk */

    /* last event id in queue should be equal to commit_event_id */
    assert (*ti()->events->commit_event_id == last_epkg->event_id);

    sprintf(buf, "%020"PRIu64".qp", last_epkg->event_id);

    fn = fx_path_join(archive->path, buf);
    if (!fn)
        goto fail0;

    f = fopen(fn, "w");
    if (!f)
        goto fail0;

    if (qp_fadd_type(f, QP_MAP_OPEN))
        goto fail1;

    for (queue_each(archive->queue, ti_epkg_t, epkg))
    {
        if (epkg->event_id > archive->last_on_disk &&
            qp_fadd_raw(f, (const uchar *) epkg->pkg, ti_pkg_sz(epkg->pkg))
        )
            goto fail1;

        if (with_sleep)
            (void) util_sleep(10);
    }

    archive->last_on_disk = last_epkg->event_id;

fail1:
    fclose(f);
    unlink(fn);

fail0:
    free(fn);
    return -1;
}

void ti_archive_cleanup(_Bool with_sleep)
{
    uint64_t m = ti_nodes_lowest_commit_event_id();
    size_t n = 0;

    if (archive->last_on_disk < m)
        m = archive->last_on_disk;

    for (queue_each(archive->queue, ti_epkg_t, epkg), ++n)
        if (epkg->event_id > m)
            break;

    if (with_sleep)
        (void) util_sleep(50);

    while (n--)
        (void *) queue_shift(archive->queue);
}
