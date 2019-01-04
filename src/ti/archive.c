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

#define ARCHIVE__FILE_FMT "%020"PRIu64".qp"
#define ARCHIVE__FILE_LEN 23
#define ARCHIVE__THRESHOLD_FULL 0

static const char * archive__path           = ".archive/";
static const char * archive__nodes_scevid    = ".nodes_scevid.dat";

static int archive__init_queue(void);
static _Bool archive__is_file(const char * fn);
static int archive__load_file(const char * fn);
static int archive__read_nodes_scevid(void);
static int archive__to_disk(void);
static int archive__remove_files(void);

static ti_archive_t * archive;


int ti_archive_create(void)
{
    archive = malloc(sizeof(ti_archive_t));
    if (!archive)
        return -1;

    archive->queue = queue_new(0);
    archive->path = NULL;
    archive->nodes_scevid_fn = NULL;
    archive->archived_on_disk = 0;
    archive->sevid = NULL;

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
    free(archive->nodes_scevid_fn);
    free(archive);
    archive = ti()->archive = NULL;
}

int ti_archive_write_nodes_scevid(void)
{
    uint64_t cevid = ti_nodes_cevid();
    uint64_t sevid = ti_nodes_sevid();
    FILE * f = fopen(archive->nodes_scevid_fn, "w");
    if (!f)
        return -1;

    log_debug("store global committed event id: "TI_EVENT_ID, cevid);
    log_debug("store global stored event id: "TI_EVENT_ID, sevid);

    if (fwrite(&cevid, sizeof(uint64_t), 1, f) != 1 ||
        fwrite(&sevid, sizeof(uint64_t), 1, f) != 1)
        goto failed;

    return fclose(f);

failed:
    fclose(f);
    return -1;
}

/*
 * Returns ti_epkg_t from ti_pkg_t
 * In case of an error this function writes logging and return NULL
 */
ti_epkg_t * ti_archive_epkg_from_pkg(ti_pkg_t * pkg)
{
    ti_epkg_t * epkg;
    qp_unpacker_t unpacker;
    qp_obj_t qp_event_id;
    uint64_t event_id;

    qp_unpacker_init(&unpacker, pkg->data, pkg->n);

    if (!qp_is_map(qp_next(&unpacker, NULL)) ||
        !qp_is_array(qp_next(&unpacker, NULL)) ||
        !qp_is_int(qp_next(&unpacker, &qp_event_id)))
    {
        log_error("invalid archive package");
        return NULL;
    }

    event_id = (uint64_t) qp_event_id.via.int64;

    epkg = ti_epkg_create(pkg, event_id);
    if (!epkg)
        log_critical(EX_ALLOC_S);

    return epkg;
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
        log_critical("cannot create archive directory: `%s`", archive->path);
        return -1;
    }

    return 0;
}

int ti_archive_load(void)
{
    ti_epkg_t * epkg;
    struct dirent ** file_list;
    int n, total;

    assert (ti()->events->cevid);
    assert (ti()->node);

    archive->start_event_id = ti()->node->cevid + 1;
    archive__read_nodes_scevid();

    total = scandir(archive->path, &file_list, NULL, alphasort);
    if (total < 0)
    {
        /* no need to free shard_list when total < 0 */
        log_critical("cannot read archive directory: `%s`.", archive->path);
        return -1;
    }

    for (n = 0; n < total; n++)
    {
        if (!archive__is_file(file_list[n]->d_name))
            continue;

        /* we are sure this fits since the filename is checked */
        if (archive__load_file(file_list[n]->d_name))
           log_critical("could not load file: `%s`", file_list[n]->d_name);
    }

    while (total--)
        free(file_list[total]);

    free(file_list);

    /* update the last stored event id */
    epkg = queue_last(archive->queue);
    if (epkg)
        *archive->sevid = epkg->event_id;

    return archive__init_queue();
}

/* increments `epkg` reference by one if successful */
int ti_archive_push(ti_epkg_t * epkg)
{
    /* queue is either empty or the received event_id > any event id inside the
     * queue
     */
    assert (
        !queue_last(archive->queue) ||
        epkg->event_id > ((ti_epkg_t *) queue_last(archive->queue))->event_id
    );
    int rc = queue_push(&archive->queue, epkg);
    if (!rc)
        ti_incref(epkg);
    return rc;
}

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
            archive->start_event_id = last_epkg->event_id + 1;
            goto success;
        }
        /* store has failed, try archive to disk */
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

static _Bool archive__is_file(const char * fn)
{
    size_t len = strlen(fn);
    if (len != ARCHIVE__FILE_LEN)
        return false;

    /* twenty digits followed by three for the file extension (.qp) */
    for (size_t i = 0; i < 20; ++i, ++fn)
        if (!isdigit(*fn))
            return false;

    return strcmp(fn, ".qp") == 0;
}

static int archive__load_file(const char * archive_fn)
{
    FILE * f;
    qp_res_t pkg_qp;
    qp_types_t qp_tp;
    char * fn = fx_path_join(archive->path, archive_fn);
    if (!fn)
        return -1;

    f = fopen(fn, "r");
    free(fn);       /* we no longer need to file name */
    if (!f)
        return -1;

    if (!qp_is_array(qp_fnext(f, NULL)))
        goto failed;

    while (qp_is_raw((qp_tp = qp_fnext(f, &pkg_qp))))
    {
        ti_epkg_t * epkg;
        ti_pkg_t * pkg = ti_pkg_dup((ti_pkg_t *) pkg_qp.via.raw->data);
        if (!pkg)
            goto failed;

        qp_res_clear(&pkg_qp);

        epkg = ti_archive_epkg_from_pkg(pkg);
        if (!epkg || (
                epkg->event_id >= archive->start_event_id &&
                queue_push(&archive->queue, epkg)))
            goto failed;

        ++archive->archived_on_disk;
    }

    if (qp_tp == QP_ERR)
        goto failed;

    return fclose(f);

failed:
    (void) fclose(f);
    return -1;
}

static int archive__read_nodes_scevid(void)
{
    assert (archive->nodes_scevid_fn);
    assert (ti()->nodes->cevid == 0);

    uint64_t cevid, sevid;
    FILE * f = fopen(archive->nodes_scevid_fn, "r");
    if (!f)
        return -1;

    if (fread(&cevid, sizeof(uint64_t), 1, f) ||
        fread(&sevid, sizeof(uint64_t), 1, f))
        goto failed;

    log_debug("known committed on all nodes: "TI_EVENT_ID, cevid);
    log_debug("known stored on all nodes: "TI_EVENT_ID, sevid);

    ti()->nodes->cevid = cevid;
    ti()->nodes->sevid = sevid;

    return fclose(f);

failed:
    fclose(f);
    return -1;
}

static int archive__to_disk(void)
{
    FILE * f;
    char * fn;
    char buf[ARCHIVE__FILE_LEN + 1];
    ti_epkg_t * last_epkg = queue_last(archive->queue);

    sprintf(buf, ARCHIVE__FILE_FMT, last_epkg->event_id);

    fn = fx_path_join(archive->path, buf);
    if (!fn)
        goto fail0;

    if (fx_file_exist(fn))
        log_error("archive file `%s` will be overwritten", fn);

    log_debug("saving event changes to: `%s`", fn);

    f = fopen(fn, "w");
    if (!f)
        goto fail0;

    if (qp_fadd_type(f, QP_ARRAY_OPEN))
        goto fail1;

    for (queue_each(archive->queue, ti_epkg_t, epkg))
    {
        if (epkg->event_id <= *archive->sevid)
            continue;

        if (qp_fadd_raw(f, (const uchar *) epkg->pkg, ti_pkg_sz(epkg->pkg)))
            goto fail1;

        ++archive->archived_on_disk;

        (void) ti_sleep(10);
    }

    if (fclose(f))
        goto fail1;

    free(fn);
    return 0;

fail1:
    (void) fclose(f);
    (void) unlink(fn);
    free(fn);

fail0:
    return -1;
}

static int archive__remove_files(void)
{
    int rc = 0;
    struct dirent * p;
    char buf[strlen(archive->path) + ARCHIVE__FILE_LEN + 1];

    DIR * d = opendir(archive->path);
    if (!d)
        return -1;

    while ((p = readdir(d)))
    {
        uint64_t event_id;

        if (!archive__is_file(p->d_name))
            continue;

        event_id = strtoull(p->d_name, NULL, 10);
        if (event_id >= archive->start_event_id)
            continue;

        (void) sprintf(buf, "%s%s", archive->path, p->d_name);
        if (unlink(buf))
        {
            rc = -1;
            log_error("unable to remove archive file: `%s`", buf);
        }
    }
    closedir(d);
    return rc;
}
