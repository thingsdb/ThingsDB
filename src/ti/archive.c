/*
 * ti/archive.c
 */
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/archfile.h>
#include <ti/changes.h>
#include <ti/cpkg.h>
#include <ti/cpkg.inline.h>
#include <unistd.h>
#include <util/fx.h>
#include <util/logger.h>
#include <util/util.h>


#define ARCHIVE__THRESHOLD_FULL 0

static const char * archive__path = "archive/";

static ti_archive_t * archive;
static ti_archive_t archive_;

static int archive__load_file(ti_archfile_t * archfile)
{
    size_t i;
    mp_unp_t up;
    mp_obj_t obj, mp_pkg;
    fx_mmap_t fmap;
    ti_cpkg_t * cpkg;

    log_debug("loading archive file `%s`", archfile->fn);

    fx_mmap_init(&fmap, archfile->fn);

    if (fx_mmap_open(&fmap))  /* fx_mmap_open() is a log function */
        return -1;

    mp_unp_init(&up, fmap.data, fmap.n);

    if (mp_next(&up, &obj) != MP_ARR)
        goto close;

    for (i = obj.via.sz; i--;)
    {
        if (mp_next(&up, &mp_pkg) != MP_BIN ||
            mp_pkg.via.bin.n < sizeof(ti_pkg_t))
        {
            log_error(
                    "failed to read archive file `%s`; ",
                    "expecting a binary `package`");
            goto close;
        }

        cpkg = ti_cpkg_from_pkg((ti_pkg_t *) mp_pkg.via.bin.data);
        if (!cpkg)  /* ti_cpkg_from_pkg() is a log function */
            goto close;

        if (cpkg->change_id <= ti.node->scid)
        {
            ti_cpkg_drop(cpkg);
        }
        else
        {
            if (queue_push(&archive->queue, cpkg))
            {
                log_critical(EX_MEMORY_S);
                goto close;
            }
            cpkg->flags |= TI_CPKG_FLAG_ALLOW_GAP;
            ti.node->scid = cpkg->change_id;
        }
    }

close:
    return fx_mmap_close(&fmap);
}

static int archive__init_queue(void)
{
    assert(ti.node);
    int rc = -1;
    ti_cpkg_t * cpkg;
    const uint64_t * ccid = &ti.node->ccid;

    if (ti.args->forget_nodes && (cpkg = queue_last(archive->queue)))
    {
        /* if we want to forget all nodes info, we should also forget changes
         * in the queue which might depend on the current nodes.
         */
        ti.last_change_id = cpkg->change_id;
        (void) ti_save();
    }

    /*
     * The cleanest way is to take all changes through the whole loop so error
     * checking is done properly, we take a lock to prevent changes being
     * processed and added to the queue.
     */
    uv_mutex_lock(ti.changes->lock);

    for (queue_each(archive->queue, ti_cpkg_t, cpkg))
        if (cpkg->change_id > *ccid)
            if (ti_changes_add_change(ti.node, cpkg) < 0)
                goto stop;

    /* remove changes from queue */
    while ((cpkg = queue_last(archive->queue)) && cpkg->change_id > *ccid)
    {
        (void) queue_pop(archive->queue);
        ti_cpkg_drop(cpkg);
    }

    rc = 0;
stop:
    uv_mutex_unlock(ti.changes->lock);

    return rc ? rc : -(ti_changes_trigger_loop() < 0);
}

static int archive__to_disk(void)
{
    assert(archive->queue->n);
    int rc = -1;
    FILE * f;
    msgpack_packer pk;
    ti_cpkg_t * cpkg;
    ti_cpkg_t * last_cpkg = queue_last(archive->queue);
    ti_archfile_t * archfile;
    const uint64_t scid = ti.node->scid;

    while ((cpkg = queue_shift(archive->queue)) && cpkg->change_id <= scid)
    {
       log_debug(
               "skip saving "TI_CHANGE_ID" because the last stored "
               "change id is higher ("TI_CHANGE_ID")",
               cpkg->change_id, scid);
       ti_cpkg_drop(cpkg);
    }

    if (!cpkg)
        return 0;  /* nothing to save to disk */

    archfile = ti_archfile_get(cpkg->change_id, last_cpkg->change_id);

    if (archfile)
        return 0;  /* these changes are already on disk */

    archfile = ti_archfile_from_change_ids(
        archive->path,
        cpkg->change_id,
        last_cpkg->change_id);

    if (!archfile)
        goto fail0;

    if (fx_file_exist(archfile->fn))
    {
        /* file should not be here, it is not in memory anyway */
        log_error("archive file `%s` will be overwritten", archfile->fn);
    }

    log_info("saving `change` data to file: `%s`", archfile->fn);

    f = fopen(archfile->fn, "w");
    if (!f)
    {
        log_errno_file("cannot open file", errno, archfile->fn);
        goto fail1;
    }

    msgpack_packer_init(&pk, f, msgpack_fbuffer_write);

    if (msgpack_pack_array(&pk, archive->queue->n + 1))
        goto fail2;

    do
    {
        assert(cpkg->change_id > scid);  /* other are removed from queue */

        if (mp_pack_bin(&pk, cpkg->pkg, ti_pkg_sz(cpkg->pkg)))
            goto fail2;

        ti_cpkg_drop(cpkg);

        (void) ti_sleep(10);
    }
    while ((cpkg = queue_shift(archive->queue)));

    if (vec_push(&archive->archfiles, archfile))
    {
        log_critical(EX_MEMORY_S);
        goto fail2;
    }

    rc = 0;

fail2:
    if (fclose(f))
    {
        log_errno_file("cannot close file", errno, archfile->fn);
        rc = -1;
    }

fail1:
    ti_cpkg_drop(cpkg);  /* cpkg = NULL when success, clean when failed */

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
    uint64_t scid = ti_nodes_scid();
    uint64_t lseid = ti.store->last_stored_change_id;
    uint64_t threshold = lseid < scid ? lseid : scid;
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
        archive->path = fx_path_join(ti.cfg->storage_path, archive__path);

    return archive->path;
}

int ti_archive_create(void)
{
    archive = &archive_;

    archive->queue = queue_new(0);
    archive->archfiles = vec_new(0);
    archive->path = NULL;

    if (!archive->queue || !archive->archfiles)
    {
        ti_archive_destroy();
        return -1;
    }

    ti.archive = archive;
    return 0;
}

void ti_archive_destroy(void)
{
    if (!archive)
        return;
    queue_destroy(archive->queue, (queue_destroy_cb) ti_cpkg_drop);
    vec_destroy(archive->archfiles, (vec_destroy_cb) ti_archfile_destroy);
    free(archive->path);
    archive = ti.archive = NULL;
}

int ti_archive_rmdir(void)
{
    int rc;
    const char * archive_path = archive__get_path();

    if (!archive_path)
    {
        log_critical(EX_MEMORY_S);
        return -1;
    }

    if (!fx_is_dir(archive_path))
        return 0;

    log_warning("removing archive directory: `%s`", archive->path);

    while (archive->archfiles->n)
        ti_archfile_destroy(VEC_pop(archive->archfiles));

    rc = fx_rmdir(archive->path);
    if (rc)
        log_error("cannot remove directory: `%s`", archive->path);

    return rc;
}

int ti_archive_init(void)
{
    assert(ti.node);

    const char * archive_path = archive__get_path();
    if (!archive_path)
        return -1;

    if (!fx_is_dir(archive_path) && mkdir(archive_path, FX_DEFAULT_DIR_ACCESS))
    {
        log_errno_file("cannot create archive directory", errno, archive_path);
        return -1;
    }

    return 0;
}

int ti_archive_load(void)
{
    struct dirent ** file_list;
    int n, total, rc = 0;
    ti_archfile_t * archfile;
    const uint64_t * ccid = &ti.node->ccid;

    log_debug("loading archive files from `%s`", archive->path);

    assert(ti.node);

    total = scandir(archive->path, &file_list, NULL, alphasort);
    if (total < 0)
    {
        /* no need to free shard_list when total < 0 */
        log_errno_file("cannot scan directory", errno, archive->path);
        return -1;
    }

    for (n = 0; n < total; n++)
    {
        if (!ti_archfile_is_valid_fn(file_list[n]->d_name))
            continue;

        archfile = ti_archfile_upsert(archive->path, file_list[n]->d_name);
        if (!archfile)
        {
            log_critical(EX_MEMORY_S);
            rc = -1;
            continue;
        }

        if (archfile->last <= *ccid)
            continue;

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

/* increments `cpkg` reference by one if successful */
int ti_archive_push(ti_cpkg_t * cpkg)
{
    /* queue is either empty or the received change_id > any change id inside
     * the queue
     */
    int rc = 0;

    assert(
        !queue_last(archive->queue) ||
        cpkg->change_id > ((ti_cpkg_t *) queue_last(archive->queue))->change_id
    );

    if (cpkg->change_id > ti.node->scid)
    {
        rc = queue_push(&archive->queue, cpkg);
        if (rc == 0)
            ti_incref(cpkg);
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
    ti_cpkg_t * last_cpkg = queue_last(archive->queue);

    if (!last_cpkg || (leid = last_cpkg->change_id) == ti.node->scid)
        goto done;       /* nothing to save to disk */

    n = leid - ti.store->last_stored_change_id;

    if (n > ti.cfg->threshold_full_storage)
        (void) ti_store_store();

    /* sleep a little before archiving */
    (void) ti_sleep(200);

    /* archive changes, even after full store for synchronizing `other` nodes */
    if (archive__to_disk())
        return -1;

    ti.node->scid = leid;  /* last_cpkg cannot be used, it's cleared */

    (void) ti_sleep(100);
done:
    (void) archive__remove_files();
    return 0;
}

/*
 * Return the first archived change id, or UINT64_MAX if no changes are inside
 * the archive. Both memory and disk are included.
 */
uint64_t ti_archive_get_first_change_id(void)
{
    ti_cpkg_t * cpkg = queue_first(archive->queue);
    uint64_t change_id = cpkg ? cpkg->change_id : UINT64_MAX;
    for (vec_each(archive->archfiles, ti_archfile_t, archfile))
        if (archfile->first < change_id)
            change_id = archfile->first;
    return change_id;
}

ti_cpkg_t * ti_archive_get_change(uint64_t change_id)
{
    ti_cpkg_t * last_cpkg = queue_last(archive->queue);

    if (!last_cpkg || last_cpkg->change_id < change_id)
        return NULL;

    if (last_cpkg->change_id == change_id)
        return last_cpkg;

    for (queue_each(archive->queue, ti_cpkg_t, cpkg))
        if (cpkg->change_id >= change_id)
            return cpkg->change_id == change_id ? cpkg : NULL;

    return NULL;
}

