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

static const char * archive__path           = ".archive/";
static const char * archive__nodes_scevid    = ".nodes_scevid.dat";

static int archive__init_queue(void);
static int archive__read_nodes_scevid(void);
static int archive__to_disk(void);
static int archive__remove_files(void);
static void archive__update_first_event_id(ti_archfile_t * archfile);

static ti_archive_t * archive;

int ti_archive_create(void)
{
    archive = malloc(sizeof(ti_archive_t));
    if (!archive)
        return -1;

    archive->queue = queue_new(0);
    archive->archfiles = queue_new(0);
    archive->path = NULL;
    archive->nodes_scevid_fn = NULL;
    archive->archived_on_disk = 0;
    archive->sevid = NULL;
    archive->first_event_id = 0;
    archive->full_stored_event_id = 0;

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
    queue_destroy(archive->archfiles, (queue_destroy_cb) ti_archfile_destroy);
    free(archive->path);
    free(archive->nodes_scevid_fn);
    free(archive);
    archive = ti()->archive = NULL;
}

int ti_archive_write_nodes_scevid(void)
{
    int rc = 0;
    uint64_t cevid = ti_nodes_cevid();
    uint64_t sevid = ti_nodes_sevid();
    char * fn = archive->nodes_scevid_fn;
    FILE * f = fopen(fn, "w");
    if (!f)
    {
        log_error("cannot open file `%s` (%s)", fn, strerror(errno));
        return -1;
    }

    log_debug("store global committed event id: "TI_EVENT_ID, cevid);
    log_debug("store global stored event id: "TI_EVENT_ID, sevid);

    if (fwrite(&cevid, sizeof(uint64_t), 1, f) != 1 ||
        fwrite(&sevid, sizeof(uint64_t), 1, f) != 1)
    {
        log_error("error writing to `%s`", fn);
        rc = -1;
    }

    if (fclose(f))
    {
        log_error("cannot close file `%s` (%s)", fn, strerror(errno));
        rc = -1;
    }
    return rc;
}

int ti_archive_init(void)
{
    struct stat st;
    char * storage_path = ti()->cfg->storage_path;

    assert (storage_path);
    assert (archive->path == NULL);
    assert (ti()->node);

    memset(&st, 0, sizeof(struct stat));

    archive->sevid = &ti()->node->sevid;
    *archive->sevid = *ti()->events->cevid;

    archive->path = fx_path_join(storage_path, archive__path);
    if (!archive->path)
        return -1;

    archive->nodes_scevid_fn = fx_path_join(
            archive->path,
            archive__nodes_scevid);
    if (!archive->nodes_scevid_fn)
        return -1;

    if (!fx_is_dir(archive->path) && mkdir(archive->path, 0700))
    {
        log_critical("cannot create archive directory: `%s` (%s)",
                archive->path,
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
    assert (ti()->events->cevid);
    assert (ti()->node);

    *archive->sevid = archive->full_stored_event_id = ti()->node->cevid;
    archive->first_event_id = archive->full_stored_event_id + 1;

    (void) archive__read_nodes_scevid();

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

        archfile = ti_archfile_create(archive->path, file_list[n]->d_name);
        if (!archfile || queue_push(&archive->archfiles, archfile))
        {
            /* can potentially leak a few bytes */
            log_critical(EX_ALLOC_S);
            rc = -1;
            continue;
        }

        /* we are sure this fits since the filename is checked */
        if (ti_archive_load_file(archfile))
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

    if (epkg->event_id > *archive->sevid)
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
    ti_epkg_t * last_epkg = queue_last(archive->queue);
    if (!last_epkg || last_epkg->event_id == *archive->sevid)
        return 0;       /* nothing to save to disk */

    /* last event id in queue should be equal to cevid */
    assert (*ti()->events->cevid == last_epkg->event_id);

    /* TODO: maybe on signal (shutdown) only store changes? */
    if (archive->archived_on_disk >= ti()->cfg->threshold_full_storage)
    {
        if (ti_store_store() == 0)
        {
            (void) archive__remove_files();
            archive->archived_on_disk = 0;
            archive->full_stored_event_id = last_epkg->event_id;
        }
    }

    if (archive__to_disk() == 0)
        goto success;

    return -1;

success:
    *archive->sevid = last_epkg->event_id;
    return 0;
}

void ti_archive_cleanup(void)
{
    uint64_t m = *archive->sevid;
    size_t n = 0;

    for (queue_each(archive->queue, ti_epkg_t, epkg), ++n)
        if (epkg->event_id > m)
            break;

    (void) ti_sleep(50);

    while (n--)
        ti_epkg_drop(queue_shift(archive->queue));
}

int ti_archive_load_file(ti_archfile_t * archfile)
{
    int rc = -1;
    FILE * f;
    qp_res_t pkg_qp;
    qp_types_t qp_tp;

    log_debug("loading archive file `%s`", archfile->fn);

    archive__update_first_event_id(archfile);

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

        if (epkg->event_id <= *archive->sevid)
        {
            ti_epkg_drop(epkg);
        }
        else
        {
            if (queue_push(&archive->queue, epkg))
                goto failed;
            *archive->sevid = epkg->event_id;
        }

        ++archive->archived_on_disk;
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
    assert (ti()->events->cevid);
    int rc = -1;
    ti_epkg_t * epkg;
    uint64_t cevid = *ti()->events->cevid;
    /*
     * The cleanest way is to take all events through the whole loop so error
     * checking is done properly, we take a lock to prevent events being
     * processed and added to the queue.
     */
    uv_mutex_lock(ti()->events->lock);

    for (queue_each(archive->queue, ti_epkg_t, epkg))
        if (epkg->event_id > cevid)
            if (ti_events_add_event(ti()->node, epkg))
                goto stop;

    /* remove events from queue */
    while ((epkg = queue_last(archive->queue)) && epkg->event_id > cevid)
    {
        (void *) queue_pop(archive->queue);
        assert (epkg->ref > 1); /* add event has created a new reference */
        ti_decref(epkg);
    }

    rc = 0;
stop:
    uv_mutex_unlock(ti()->events->lock);

    return rc ? rc : ti_events_trigger_loop();
}



static int archive__read_nodes_scevid(void)
{
    assert (archive->nodes_scevid_fn);
    assert (ti()->nodes->cevid == 0);

    int rc = -1;
    uint64_t cevid, sevid;
    char * fn = archive->nodes_scevid_fn;
    FILE * f = fopen(fn, "r");
    if (!f)
    {
        log_info("cannot open file `%s` (%s)", fn, strerror(errno));
        return -1;
    }

    if (fread(&cevid, sizeof(uint64_t), 1, f) ||
        fread(&sevid, sizeof(uint64_t), 1, f))
        goto stop;

    log_debug("known committed on all nodes: "TI_EVENT_ID, cevid);
    log_debug("known stored on all nodes: "TI_EVENT_ID, sevid);

    ti()->nodes->cevid = cevid;
    ti()->nodes->sevid = sevid;
    rc = 0;

stop:
    if (fclose(f))
    {
        log_error("cannot close file `%s` (%s)", fn, strerror(errno));
        rc = -1;
    }
    return rc;
}

static int archive__to_disk(void)
{
    assert (archive->queue->n);
    int rc = -1;
    FILE * f;
    ti_epkg_t * first_epkg = queue_first(archive->queue);
    ti_epkg_t * last_epkg = queue_last(archive->queue);
    ti_archfile_t * archfile = ti_archfile_from_event_ids(
            archive->path,
            first_epkg->event_id,
            last_epkg->event_id);
    if (!archfile)
        goto fail0;

    if (fx_file_exist(archfile->fn))
        log_error("archive file `%s` will be overwritten", archfile->fn);

    log_debug("saving event changes to: `%s`", archfile->fn);

    f = fopen(archfile->fn, "w");
    if (!f)
    {
        log_error("cannot open file `%s` (%s)", archfile->fn, strerror(errno));
        goto fail1;
    }

    if (qp_fadd_type(f, QP_ARRAY_OPEN))
        goto fail2;

    for (queue_each(archive->queue, ti_epkg_t, epkg))
    {
        if (epkg->event_id <= *archive->sevid)
        {
            log_warning(
                    "storing "TI_EVENT_ID" while the last stored "
                    "event id is higher ("TI_EVENT_ID")",
                    epkg->event_id, *archive->sevid);
        }

        if (qp_fadd_raw(f, (const uchar *) epkg->pkg, ti_pkg_sz(epkg->pkg)))
            goto fail2;

        ++archive->archived_on_disk;

        (void) ti_sleep(10);
    }

    if (queue_push(&archive->archfiles, archfile))
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
    _Bool found;

    /* Reset archive first event id */
    archive->first_event_id = archive->full_stored_event_id + 1;

    do
    {
        found = false;
        size_t idx = 0;
        for (queue_each(archive->archfiles, ti_archfile_t, archfile), ++idx)
        {
            if (archfile->last <= sevid)
            {
                log_debug("removing archive file `%s`", archfile->fn);
                if (unlink(archfile->fn))
                {
                    rc = -1;
                    log_error("unable to remove archive file: `%s`",
                            archfile->fn);
                }

                (void *) queue_remove(archive->archfiles, idx);
                ti_archfile_destroy(archfile);

                found = true;
                break;
            }

            archive__update_first_event_id(archfile);
        }
    }
    while (found);

    return rc;
}

static void archive__update_first_event_id(ti_archfile_t * archfile)
{
    if (archfile->first < archive->first_event_id)
    {
        log_debug(
                "update first event id from "TI_EVENT_ID" to "TI_EVENT_ID,
                archive->first_event_id,
                archfile->first);
        archive->first_event_id = archfile->first;
    }
}

