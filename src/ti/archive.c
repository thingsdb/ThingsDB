/*
 * ti/archive.c
 */
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <ti/epkg.h>
#include <ti.h>
#include <util/logger.h>
#include <util/fx.h>
#include <util/util.h>
#include <unistd.h>

static const char * archive__path           = ".archive/";
static const char * archive__nodes_cevid    = ".nodes_cevid.dat";

static int archive__init_queue(void);
static _Bool archive__is_file(const char * fn);
static int archive__load_file(const char * fn);
static int archive__read_nodes_cevid(void);

static ti_archive_t * archive;


int ti_archive_create(void)
{
    archive = malloc(sizeof(ti_archive_t));
    if (!archive)
        return -1;

    archive->queue = queue_new(0);
    archive->path = NULL;
    archive->events_in_achive = 0;
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

int ti_archive_write_nodes_cevid(void)
{
    uint64_t cevid = ti()->nodes->cevid;
    FILE * f = fopen(archive->nodes_cevid_fn, "w");
    if (!f)
        return -1;

    if (fwrite(&cevid, sizeof(uint64_t), 1, f))
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

int ti_archive_load(void)
{
    struct stat st;
    struct dirent ** file_list;
    int n, total;
    char * storage_path = ti()->cfg->storage_path;

    assert (storage_path);
    assert (archive->path == NULL);
    assert (ti()->events->cevid);

    memset(&st, 0, sizeof(struct stat));

    archive->path = fx_path_join(storage_path, archive__path);
    if (!archive->path)
        return -1;

    archive->nodes_cevid_fn = fx_path_join(
            archive->path,
            archive__nodes_cevid);
    if (!archive->nodes_cevid_fn)
        return -1;

    archive__read_nodes_cevid();

    if (!fx_is_dir(archive->path) && mkdir(archive->path, 0700))
    {
        log_critical("cannot create archive directory: `%s`", archive->path);
        return -1;
    }

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

    return 0;
}

int ti_archive_push(ti_epkg_t * epkg)
{
    int rc = queue_push(&archive->queue, epkg);
    if (!rc)
        ++archive->events_in_achive;
    return rc;
}

int ti_archive_to_disk(void)
{
    FILE * f;
    char * fn;
    char buf[24];
    ti_epkg_t * last_epkg = queue_last(archive->queue);
    if (!last_epkg || last_epkg->event_id == archive->last_on_disk)
        return 0;       /* nothing to save to disk */

    /* last event id in queue should be equal to cevid */
    assert (*ti()->events->cevid == last_epkg->event_id);

    sprintf(buf, "%020"PRIu64".qp", last_epkg->event_id);

    fn = fx_path_join(archive->path, buf);
    if (!fn)
        goto fail0;

    log_debug("saving event changes to: `%s`", fn);

    f = fopen(fn, "w");
    free(fn);   /* we no longer need the file name */
    if (!f)
        goto fail0;

    if (qp_fadd_type(f, QP_ARRAY_OPEN))
        goto fail1;

    for (queue_each(archive->queue, ti_epkg_t, epkg))
    {
        if (epkg->event_id > archive->last_on_disk &&
            qp_fadd_raw(f, (const uchar *) epkg->pkg, ti_pkg_sz(epkg->pkg))
        )
            goto fail1;

        (void) ti_sleep(10);
    }

    if (fclose(f))
        goto fail1;

    archive->last_on_disk = last_epkg->event_id;
    return 0;

fail1:
    (void) fclose(f);
    (void) unlink(fn);

fail0:
    return -1;
}

void ti_archive_cleanup(void)
{
    uint64_t m = ti()->nodes->cevid;
    size_t n = 0;

    if (archive->last_on_disk < m)
        m = archive->last_on_disk;

    for (queue_each(archive->queue, ti_epkg_t, epkg), ++n)
        if (epkg->event_id > m)
            break;

    (void) ti_sleep(50);

    while (n--)
        (void *) queue_shift(archive->queue);
}

static int archive__init_queue(void)
{
    return 0;
}

static _Bool archive__is_file(const char * fn)
{
    size_t len = strlen(fn);
    if (len != 23)
        return false;

    for (size_t i = 0; i < 20; ++i, ++fn)
        if (!isdigit(*fn))
            return false;

    return strcmp(fn, ".qp") == 0;
}

static int archive__load_file(const char * archive_fn)
{
    FILE * f;
    qp_res_t pkg_qp;
    uint64_t cevid = ti()->nodes->cevid;
    char * fn = fx_path_join(archive->path, archive_fn);
    if (!fn)
        return -1;

    f = fopen(fn, "r");
    free(fn);       /* we no longer need to file name */
    if (!f)
        return -1;

    if (!qp_is_array(qp_fnext(f, NULL)))
        goto failed;

    while (qp_is_raw(qp_fnext(f, &pkg_qp)))
    {
        ti_epkg_t * epkg;
        ti_pkg_t * pkg = ti_pkg_dup((ti_pkg_t *) pkg_qp.via.raw->data);
        if (!pkg)
            goto failed;

        epkg = ti_archive_epkg_from_pkg(pkg);
        if (!epkg || (
                epkg->event_id > cevid && queue_push(&archive->queue, epkg)))
            goto failed;

        ++archive->events_in_achive;
    }

    return fclose(f);

failed:
    (void) fclose(f);
    return -1;
}

static int archive__read_nodes_cevid(void)
{
    assert (archive->nodes_cevid_fn);
    assert (ti()->nodes->cevid == 0);

    uint64_t cevid;
    FILE * f = fopen(archive->nodes_cevid_fn, "r");
    if (!f)
        return -1;

    if (fread(&cevid, sizeof(uint64_t), 1, f))
        goto failed;

    log_debug("known committed event id on all nodes: `%"PRIu64"`", cevid);

    ti()->nodes->cevid = cevid;

    return fclose(f);

failed:
    fclose(f);
    return -1;
}
