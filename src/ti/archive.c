/*
 * ti/archive.c
 */
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <ti/epkg.h>
#include <ti/events.h>
#include <ti.h>
#include <util/logger.h>
#include <util/fx.h>
#include <util/util.h>
#include <unistd.h>


#define ARCHIVE__THRESHOLD_FULL 0

static const char * archive__path           = "archive/";

static int archive__load_file(ti_archfile_t * archfile);
static int archive__init_queue(void);
static int archive__to_disk(void);
static int archive__remove_files(void);
static char * archive__get_path(void);

static ti_archive_t * archive;

int ti_archive_create(void)
{
    archive = malloc(sizeof(ti_archive_t));
    if (!archive)
        return -1;

    archive->queue = queue_new(0);
    archive->archfiles = vec_new(0);
    archive->path = NULL;

    if (!archive->queue || !archive->archfiles)
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
    vec_destroy(archive->archfiles, (vec_destroy_cb) ti_archfile_destroy);
    free(archive->path);
    free(archive);
    archive = ti()->archive = NULL;
}


int ti_archive_rmdir(void)
{
    int rc;
    const char * archive_path = archive__get_path();

    if (!archive_path)
    {
        log_critical(EX_ALLOC_S);
        return -1;
    }

    if (!fx_is_dir(archive_path))
        return 0;

    log_warning("removing archive directory: `%s`", archive->path);

    while (archive->archfiles->n)
        ti_archfile_destroy(vec_pop(archive->archfiles));

    rc = fx_rmdir(archive->path);
    if (rc)
        log_error("cannot remove directory: `%s`", archive->path);

    return rc;
}

int ti_archive_init(void)
{
    assert (ti()->node);

    const char * archive_path = archive__get_path();
    if (!archive_path)
        return -1;

    if (!fx_is_dir(archive_path) && mkdir(archive_path, 0700))
    {
        log_critical("cannot create archive directory: `%s` (%s)",
                archive_path,
                strerror(errno));
        return -1;
    }

    return 0;
}

int ti_archive_load(void)
{
    struct dirent ** file_list;
    int n, total, rc = 0;
    ti_archfile_t * archfile;

    log_debug("loading archive files from `%s`", archive->path);

    assert (ti()->node);

    total = scandir(archive->path, &file_list, NULL, alphasort);
    if (total < 0)
    {
        /* no need to free shard_list when total < 0 */
        log_critical(
                "cannot scan directory `%s` (%s)",
                archive->path,
                strerror(errno));
        return -1;
    }

    for (n = 0; n < total; n++)
    {
        if (!ti_archfile_is_valid_fn(file_list[n]->d_name))
            continue;

        archfile = ti_archfile_upsert(archive->path, file_list[n]->d_name);
        if (!archfile)
        {
            log_critical(EX_ALLOC_S);
            rc = -1;
            continue;
        }

        /* we are sure this fits since the filename is checked */
        if (archive__load_file(archfile))
        {
           log_critical("could not load file: `%s`", file_list[n]->d_name);
           rc = -1;
        }
    }

    while (total--)
        free(file_list[total]);

    free(file_list);

    return rc ? rc : archive__init_queue();
}

/* increments `epkg` reference by one if successful */
int ti_archive_push(ti_epkg_t * epkg)
{
    /* queue is either empty or the received event_id > any event id inside the
     * queue
     */
    int rc = 0;

    assert (
        !queue_last(archive->queue) ||
        epkg->event_id > ((ti_epkg_t *) queue_last(archive->queue))->event_id
    );

    if (epkg->event_id > ti()->node->sevid)
    {
        rc = queue_push(&archive->queue, epkg);
        if (rc == 0)
            ti_incref(epkg);
    }
    return rc;
}

/*
 * May run from the `away->work` thread
 */
int ti_archive_to_disk(void)
{
    size_t n;
    uint64_t leid;
    ti_epkg_t * last_epkg = queue_last(archive->queue);

    if (!last_epkg || (leid = last_epkg->event_id) == ti()->node->sevid)
        goto done;       /* nothing to save to disk */

    n = leid - ti()->store->last_stored_event_id;

    if (n >= ti()->cfg->threshold_full_storage)
        (void) ti_store_store();

    /* archive events, even after full store for synchronizing `other` nodes */
    if (archive__to_disk())
        return -1;

    ti()->node->sevid = leid;  /* last_epkg cannot be used, it's cleared */

done:
    (void) archive__remove_files();
    return 0;
}

/*
 * Return the first archived event id, or UINT64_MAX if no events are inside
 * the archive. Both memory and disk are included.
 */
uint64_t ti_archive_get_first_event_id(void)
{
    ti_epkg_t * epkg = queue_first(archive->queue);
    uint64_t event_id = epkg ? epkg->event_id : UINT64_MAX;
    for (vec_each(archive->archfiles, ti_archfile_t, archfile))
        if (archfile->first < event_id)
            event_id = archfile->first;
    return event_id;
}

static int archive__load_file(ti_archfile_t * archfile)
{
    int rc = -1;
    FILE * f;
    qp_res_t pkg_qp;
    qp_types_t qp_tp;
    ti_event_t * event = queue_first(ti()->events->queue);

    log_debug("loading archive file `%s`", archfile->fn);

    f = fopen(archfile->fn, "r");
    if (!f)
    {
        log_error("cannot open file `%s` (%s)", archfile->fn, strerror(errno));
        return rc;
    }

    if (!qp_is_array(qp_fnext(f, NULL)))
        goto failed;

    while (qp_is_raw((qp_tp = qp_fnext(f, &pkg_qp))))
    {
        ti_epkg_t * epkg = ti_epkg_from_pkg((ti_pkg_t *) pkg_qp.via.raw->data);

        qp_res_clear(&pkg_qp);

        if (!epkg)
            goto failed;

        if (epkg->event_id <= ti()->node->sevid ||
                (event && epkg->event_id >= event->id))
        {
            ti_epkg_drop(epkg);
        }
        else
        {
            if (queue_push(&archive->queue, epkg))
                goto failed;

            ti()->node->sevid = epkg->event_id;
        }
    }

    if (qp_tp == QP_ERR)
        goto failed;

    rc = 0;

failed:
    if (fclose(f))
    {
        log_error("cannot close file `%s` (%s)",
                archfile->fn, strerror(errno));
        rc = -1;
    }

    return rc;
}

static int archive__init_queue(void)
{
    assert (ti()->node);
    int rc;
    ti_epkg_t * epkg;
    const uint64_t cevid = ti()->node->cevid;
    /*
     * The cleanest way is to take all events through the whole loop so error
     * checking is done properly, we take a lock to prevent events being
     * processed and added to the queue.
     */
    uv_mutex_lock(ti()->events->lock);

    for (queue_each(archive->queue, ti_epkg_t, epkg))
        if (epkg->event_id > cevid)
            if (ti_events_add_event(ti()->node, epkg) < 0)
                goto stop;

    /* remove events from queue */
    while ((epkg = queue_last(archive->queue)) && epkg->event_id > cevid)
    {
        (void) queue_pop(archive->queue);
        assert (epkg->ref > 1); /* add event has created a new reference */
        ti_decref(epkg);
    }

    rc = 0;
stop:
    uv_mutex_unlock(ti()->events->lock);

    return rc ? rc : -(ti_events_trigger_loop() < 0);
}

static int archive__to_disk(void)
{
    assert (archive->queue->n);
    int rc = -1;
    FILE * f;
    ti_epkg_t * epkg;
    ti_epkg_t * last_epkg = queue_last(archive->queue);
    ti_archfile_t * archfile;
    const uint64_t sevid = ti()->node->sevid;

    while ((epkg = queue_shift(archive->queue)) && epkg->event_id <= sevid)
    {
       log_debug(
               "skip saving "TI_EVENT_ID" because the last stored "
               "event id is higher ("TI_EVENT_ID")",
               epkg->event_id, sevid);
       ti_epkg_drop(epkg);
    }

    if (!epkg)
        return 0;  /* nothing to save to disk */

    archfile = ti_archfile_get(epkg->event_id, last_epkg->event_id);

    if (archfile)
        return 0;  /* these events are already on disk */

    archfile = ti_archfile_from_event_ids(
        archive->path,
        epkg->event_id,
        last_epkg->event_id);

    if (!archfile)
        goto fail0;

    if (fx_file_exist(archfile->fn))
    {
        /* file should not be here, it is not in memory anyway */
        log_error("archive file `%s` will be overwritten", archfile->fn);
    }

    log_info("saving event changes to file: `%s`", archfile->fn);

    f = fopen(archfile->fn, "w");
    if (!f)
    {
        log_error("cannot open file `%s` (%s)", archfile->fn, strerror(errno));
        goto fail1;
    }

    if (qp_fadd_type(f, QP_ARRAY_OPEN))
        goto fail2;

    do
    {
        assert (epkg->event_id > sevid);  /* other are removed from queue */

        if (qp_fadd_raw(f, (const uchar *) epkg->pkg, ti_pkg_sz(epkg->pkg)))
            goto fail2;

        ti_epkg_drop(epkg);

        (void) ti_sleep(10);
    }
    while ((epkg = queue_shift(archive->queue)));

    if (vec_push(&archive->archfiles, archfile))
    {
        log_critical(EX_ALLOC_S);
        goto fail2;
    }

    rc = 0;

fail2:
    if (fclose(f))
    {
        log_error("cannot close file `%s` (%s)",
                archfile->fn, strerror(errno));
        rc = -1;
    }

fail1:
    if (rc)
    {
        (void) unlink(archfile->fn);
        ti_archfile_destroy(archfile);
    }

fail0:
    return rc;
}

static int archive__remove_files(void)
{
    int rc = 0;
    uint64_t sevid = ti_nodes_sevid();
    uint64_t lseid = ti()->store->last_stored_event_id;
    uint64_t threshold = lseid < sevid ? lseid : sevid;
    _Bool found;

    do
    {
        found = false;
        size_t idx = 0;
        for (vec_each(archive->archfiles, ti_archfile_t, archfile), ++idx)
        {
            if (archfile->last <= threshold)
            {
                log_info("removing archive file: `%s`", archfile->fn);
                if (unlink(archfile->fn))
                {
                    rc = -1;
                    log_error("unable to remove archive file: `%s`",
                            archfile->fn);
                }

                (void) vec_swap_remove(archive->archfiles, idx);
                ti_archfile_destroy(archfile);

                found = true;
                break;
            }
        }
    }
    while (found);

    return rc;
}

static char * archive__get_path(void)
{
    if (!archive->path)
        archive->path = fx_path_join(ti()->cfg->storage_path, archive__path);

    return archive->path;
}
