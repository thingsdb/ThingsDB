/*
 * ti/backups.h
 */
#include <stdlib.h>
#include <ti.h>
#include <ti/backup.h>
#include <ti/backups.h>
#include <ti/raw.inline.h>
#include <ti/val.inline.h>
#include <util/buf.h>
#include <util/fx.h>
#include <util/util.h>
#include <util/vec.h>

static const char * backups__fn             = "node_backups.mp";

static ti_backups_t * backups;
static ti_backups_t backups_;

static int backups__gcd_rm(ti_raw_t * fn)
{
    int rc = -1;
    char buffer[512];
    FILE * fp;
    buf_t buf;
    buf_init(&buf);

    if (ti.cfg->gcloud_key_file)
        buf_append_fmt(
                &buf,
                "gcloud auth activate-service-account "
                "--quiet --key-file \"%s\" 2>&1; ",
                ti.cfg->gcloud_key_file);

    buf_append_fmt(
            &buf,
            "gsutil -o 'Boto:num_retries=1' rm %.*s 2>&1;",
            fn->n, (char *) fn->data);

    if (buf_write(&buf, '\0'))
        goto fail0;

    fp = popen(buf.data, "r");
    if (!fp)
        goto fail0;

    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        size_t sz = strlen(buffer);
        if (sz)
            log_debug(buffer);
    }

    rc = pclose(fp);

fail0:
    free(buf.data);
    return rc;
}

static void backups__rm(uv_work_t * work)
{
    ti_raw_t * gs_str = (ti_raw_t *) ti_val_borrow_gs_str();
    vec_t * vec = work->data;

    for (vec_each(vec, ti_raw_t, fn))
    {
        if (ti_raw_startswith(fn, gs_str))
            (void) backups__gcd_rm(fn);
        else
            (void) fx_unlink_n((const char *) fn->data, fn->n);
    }
}

static void backups__rm_finish(uv_work_t * work, int status)
{
    vec_t * vec = work->data;

    if (status)
        log_error(uv_strerror(status));

    vec_destroy(vec, (vec_destroy_cb) ti_val_unsafe_drop);
    free(work);
}

static void backups__delete_files(ti_backup_t * backup)
{
    vec_t * vec;
    uv_work_t * work;

    if (!backup->files->n)
        return;

    vec = vec_new(backup->files->n);
    if (!vec)
        goto fail0;

    for (queue_each(backup->files, ti_raw_t, fn))
    {
        ti_incref(fn);
        VEC_push(vec, fn);
    }

    work = malloc(sizeof(uv_work_t));
    if (!work)
        goto fail1;

    work->data = vec;

    if (uv_queue_work(ti.loop, work, backups__rm, backups__rm_finish))
        goto fail2;

    return;

fail2:
    free(work);
fail1:
    vec_destroy(vec, (vec_destroy_cb) ti_val_unsafe_drop);
fail0:
    log_error("failed to remove files for backup id %u", backup->id);
    return;
}

static void backup__delete_file(ti_raw_t * fn)
{
    vec_t * vec;
    uv_work_t * work;

    vec = vec_new(1);
    if (!vec)
        goto fail0;

    ti_incref(fn);
    VEC_push(vec, fn);

    work = malloc(sizeof(uv_work_t));
    if (!work)
        goto fail1;

    work->data = vec;

    if (uv_queue_work(ti.loop, work, backups__rm, backups__rm_finish))
        goto fail2;

    return;

fail2:
    free(work);
fail1:
    vec_destroy(vec, (vec_destroy_cb) ti_val_unsafe_drop);
fail0:
    log_error("failed to remove backup file: `%.*s`",
            fn->n, (char *) fn->data);
    return;
}

int ti_backups_create(void)
{
    backups = &backups_;

    backups->lock =  malloc(sizeof(uv_mutex_t));
    backups->fn = NULL;
    backups->next_id = 0;

    backups->omap = omap_create();
    backups->changed = false;

    if (!backups->omap ||
        !backups->lock ||
        uv_mutex_init(backups->lock))
    {
        ti_backups_destroy();
        return -1;
    }

    ti.backups = backups;
    return 0;
}

void ti_backups_destroy(void)
{
    if (!backups)
        return;
    omap_destroy(backups->omap, (omap_destroy_cb) ti_backup_destroy);
    uv_mutex_destroy(backups->lock);
    free(backups->lock);
    free(backups->fn);
    ti.backups = NULL;
}

/* this function requires a lock since it returns a `backup` which requires
 * a lock by itself;
 *
 * argument `id` is not exact but checks for "at least" this backup id. This
 * is useful so the ge_pending function can be used and prevent returning the
 * same backup more than once.
 */
static ti_backup_t * backups__get_pending(uint64_t ts, uint64_t at_least_id)
{
    omap_iter_t iter = omap_iter(backups->omap);
    for (omap_each(iter, ti_backup_t, backup))
        if (backup->scheduled &&
            backup->id >= at_least_id &&
            backup->next_run < ts)
            return backup;
    return NULL;
}

void ti_backups_upd_status(uint64_t backup_id, int rc, buf_t * buf)
{
    uint64_t now = util_now_usec();
    ti_backup_t * backup;
    ti_raw_t * last_fn;

    if (rc)
        log_error("backup result: %.*s", buf->len, buf->data);
    else
        log_info("backup result: %.*s", buf->len, buf->data);

    uv_mutex_lock(backups->lock);

    backup = omap_get(backups->omap, backup_id);

    if (!backup)
        goto done;

    free(backup->result_msg);
    backup->result_msg = strndup(buf->data, buf->len);

    backup->result_code = rc;

    if (rc || (
            (last_fn = queue_last(backup->files)) &&
            ti_raw_eq(last_fn, backup->work_fn)))
    {
        ti_val_drop((ti_val_t *) backup->work_fn);
    }
    else
    {
        if (backup->files->n == backup->max_files)
        {
            ti_raw_t * first_fn = queue_shift(backup->files);

            assert (first_fn);  /* max_files > 0 so we always have some file */

            backup__delete_file(first_fn);

            /* drop the file name */
            ti_val_unsafe_drop((ti_val_t *) first_fn);
        }

        /*
         * The queue always has enough space since the queue_size is at
         * least equal to max_files.
         */
        QUEUE_push(backup->files, backup->work_fn);
    }

    backup->work_fn = NULL;  /* either dropped or moved to the queue */

    if (backup->repeat)
    {
        do
            backup->next_run += backup->repeat;
        while (backup->next_run < now);
    }
    else
        backup->scheduled = false;

    backups->changed = true;

done:
    uv_mutex_unlock(backups->lock);
}

static void backups__run(uint64_t backup_id, const char * backup_task)
{
    char buffer[512];
    int rc = -1;
    FILE * fp;
    buf_t buf;
    buf_init(&buf);

    log_debug(backup_task);

    fp = popen(backup_task, "r");
    if (!fp)
    {
        buf_append_str(&buf, "failed to open `backup` task");
    }
    else
    {
        while (fgets(buffer, sizeof(buffer), fp) != NULL)
        {
            size_t sz = strlen(buffer);
            if (sz)
                buf_append(&buf, buffer, sz);
        }

        rc = pclose(fp);

        log_debug("%.*s", buf.len, buf.data);

        if (rc == 0)
        {
            uint64_t now = util_now_usec();
            struct tm * tm_info;
            tm_info = gmtime((const time_t *) &now);

            free(buf.data);
            buf_init(&buf);

            buf_append_str(&buf, "success - ");

            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%SZ", tm_info);
            buf_append_str(&buf, buffer);
        }
    }

    ti_backups_upd_status(backup_id, rc, &buf);
    free(buf.data);
}

static int backups__store(void)
{
    omap_iter_t iter;
    msgpack_packer pk;
    FILE * f = fopen(backups->fn, "w");
    static char empty[1] = "";
    char * result_msg;
    if (!f)
    {
        log_errno_file("cannot open file", errno,  backups->fn);
        return -1;
    }

    msgpack_packer_init(&pk, f, msgpack_fbuffer_write);

    uv_mutex_lock(backups->lock);

    iter = omap_iter(backups->omap);

    if (
        msgpack_pack_map(&pk, 1) ||
        mp_pack_str(&pk, "backups") ||
        msgpack_pack_array(&pk, backups->omap->n)
    ) goto fail;

    for (omap_each(iter, ti_backup_t, backup))
    {
        result_msg = backup->result_msg ? backup->result_msg : empty;
        if (msgpack_pack_array(&pk, 10) ||
            msgpack_pack_uint64(&pk, backup->id) ||
            msgpack_pack_uint64(&pk, backup->created_at) ||
            msgpack_pack_uint64(&pk, backup->next_run) ||
            msgpack_pack_uint64(&pk, backup->repeat) ||
            msgpack_pack_uint64(&pk, backup->max_files) ||
            mp_pack_str(&pk, backup->fn_template) ||
            (backup->result_msg
                    ? mp_pack_str(&pk, result_msg)
                    : msgpack_pack_nil(&pk)) ||
            mp_pack_bool(&pk, backup->scheduled) ||
            msgpack_pack_fix_int32(&pk, backup->result_code) ||
            msgpack_pack_array(&pk, backup->files->n)
        ) goto fail;

        for (queue_each(backup->files, ti_raw_t, fn))
            if (mp_pack_strn(&pk, fn->data, fn->n))
                goto fail;
    }

    log_debug("stored backup schedules to file: `%s`", backups->fn);
    goto done;
fail:
    log_error("failed to write file: `%s`", backups->fn);
done:
    uv_mutex_unlock(backups->lock);


    if (fclose(f))
    {
        log_errno_file("cannot close file", errno, backups->fn);
        return -1;
    }

    backups->changed = false;

    return 0;
}

static int backups__ensure_fn(void)
{
    if (backups->fn)
        return 0;

    backups->fn = fx_path_join(ti.cfg->storage_path, backups__fn);
    return backups->fn ? 0 : -1;
}

int ti_backups_rm(void)
{
    if (backups__ensure_fn())
        return -1;

    if (fx_file_exist(ti.backups->fn))
    {
        log_warning("removing backup schedules: `%s`", ti.backups->fn);
        (void) unlink(ti.backups->fn);
    }
    return 0;
}

int ti_backups_restore(void)
{
    int rc = -1;
    fx_mmap_t fmap;
    size_t i, ii;
    mp_obj_t obj, arr, mp_ver, mp_id, mp_ts, mp_repeat, mp_template,
             mp_fn, mp_msg, mp_plan, mp_code, mp_created, mp_max_files;
    mp_unp_t up;
    ti_backup_t * backup;
    uint64_t now = util_now_usec();
    queue_t * files_queue;
    ti_raw_t * raw_fn;
    _Bool set_changed = false;

    /* prevents a compiler warning */
    mp_msg.via.str.n = 0;

    if (backups__ensure_fn())
        return -1;

    if (!fx_file_exist(backups->fn))
        return 0;

    fx_mmap_init(&fmap, backups->fn);
    if (fx_mmap_open(&fmap))  /* fx_mmap_open() is a log function */
        goto fail0;

    mp_unp_init(&up, fmap.data, fmap.n);

    if (mp_next(&up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_next(&up, &mp_ver) != MP_STR ||
        mp_next(&up, &obj) != MP_ARR)
        goto fail1;

    for (i = obj.via.sz; i--;)
    {
        if (mp_next(&up, &obj) != MP_ARR)
            goto fail1;

        switch (obj.via.sz)

        {
        case 8:
            /*
             * TODO: (COMPAT) Before v0.9.9 backups are stored with a
             *       max_files value and files queue. This code may be
             *       removed once we want to drop backwards compatibility.
             */
            if (mp_next(&up, &mp_id) != MP_U64 ||
                mp_next(&up, &mp_created) != MP_U64 ||
                mp_next(&up, &mp_ts) != MP_U64 ||
                mp_next(&up, &mp_repeat) != MP_U64 ||
                mp_next(&up, &mp_template) != MP_STR ||
                mp_next(&up, &mp_msg) <= 0  ||
                mp_next(&up, &mp_plan) != MP_BOOL ||
                mp_next(&up, &mp_code) != MP_I64
            ) goto fail1;
            mp_max_files.tp = MP_U64;
            mp_max_files.via.u64 = mp_repeat.via.u64
                    ? TI_BACKUP_DEFAULT_MAX_FILES
                    : 1;
            files_queue = queue_new(mp_max_files.via.u64);
            if (!files_queue)
                goto fail1;
            set_changed = true;
            break;
        case 10:
            if (mp_next(&up, &mp_id) != MP_U64 ||
                mp_next(&up, &mp_created) != MP_U64 ||
                mp_next(&up, &mp_ts) != MP_U64 ||
                mp_next(&up, &mp_repeat) != MP_U64 ||
                mp_next(&up, &mp_max_files) != MP_U64 ||
                mp_next(&up, &mp_template) != MP_STR ||
                mp_next(&up, &mp_msg) <= 0  ||
                mp_next(&up, &mp_plan) != MP_BOOL ||
                mp_next(&up, &mp_code) != MP_I64 ||
                mp_next(&up, &arr) != MP_ARR
            ) goto fail1;
            files_queue = queue_new(mp_max_files.via.u64);
            if (!files_queue)
                goto fail1;

            for (ii = arr.via.sz; ii--;)
            {
                if (mp_next(&up, &mp_fn) != MP_STR ||
                    !(raw_fn = ti_str_create(
                            (const char *) mp_fn.via.str.data,
                            mp_fn.via.str.n)))
                {
                    queue_destroy(
                            files_queue,
                            (queue_destroy_cb) ti_val_unsafe_drop);
                    goto fail1;
                }

                QUEUE_push(files_queue, raw_fn);
            }

            break;
        default:
            goto fail1;
        }

        backup = ti_backups_new_backup(
                mp_id.via.u64,
                mp_template.via.str.data,
                mp_template.via.str.n,
                mp_ts.via.u64,
                mp_repeat.via.u64,
                mp_max_files.via.u64,
                mp_created.via.u64,
                files_queue);

        if (backup)
        {
            backup->result_msg = mp_msg.tp == MP_STR
                    ? strndup(mp_msg.via.str.data, mp_msg.via.str.n)
                    : NULL;
            backup->result_code = (int) mp_code.via.i64;
            backup->scheduled = mp_plan.via.bool_;
            if (backup->repeat)
                while (backup->next_run < now)
                    backup->next_run += backup->repeat;
        }
        else
        {
            queue_destroy(files_queue, (queue_destroy_cb) ti_val_unsafe_drop);
        }
    }

    /* set here since a new backup always sets the value to true */
    backups->changed = set_changed;
    rc = 0;

fail1:
    if (fx_mmap_close(&fmap))
        rc = -1;
fail0:
    if (rc)
        log_error("failed to restore from file: `%s`", backups->fn);

    /*
     * do not return rc`, but 0 since nothing except the file name is critical
     */
    return 0;
}

int ti_backups_store(void)
{
    return backups->changed ? backups__store() : 0;
}

int ti_backups_backup(void)
{
    char * backup_task;
    ti_backup_t * backup;
    uint64_t now = util_now_usec();
    uint64_t backup_id = 0;  /* At least backup Id...*/

    do
    {
        (void) ti_sleep(100);

        uv_mutex_lock(backups->lock);

        backup_task = NULL;
        backup = backups__get_pending(now, backup_id);
        if (backup)
        {
            backup_id = backup->id;
            backup_task = ti_backup_is_gcloud(backup)
                    ? ti_backup_gcloud_task(backup)
                    : ti_backup_task(backup);
        }

        uv_mutex_unlock(backups->lock);

        if (!backup_task)
            break;

        backups__run(backup_id, backup_task);
        free(backup_task);
        ++backup_id;

    } while(1);

    if (backups->changed)
    {
        (void) backups__store();
    }

    return 0;
}

size_t ti_backups_scheduled(void)
{
    size_t n = 0;
    omap_iter_t iter;

    uv_mutex_lock(backups->lock);

    iter = omap_iter(backups->omap);
    for (omap_each(iter, ti_backup_t, backup))
        if (backup->scheduled)
            ++n;

    uv_mutex_unlock(backups->lock);

    return n;
}

size_t ti_backups_pending(void)
{
    uint64_t now = util_now_usec();
    size_t n = 0;
    omap_iter_t iter;


    uv_mutex_lock(backups->lock);

    iter = omap_iter(backups->omap);
    for (omap_each(iter, ti_backup_t, backup))
        if (backup->scheduled && backup->next_run < now)
            ++n;

    uv_mutex_unlock(backups->lock);

    return n;
}

_Bool ti_backups_require_away(void)
{
    return backups->changed || ti_backups_pending() > 0;
}

ti_varr_t * ti_backups_info(void)
{
    omap_iter_t iter;
    ti_varr_t * varr = ti_varr_create(backups->omap->n);
    if (!varr)
        return NULL;

    uv_mutex_lock(backups->lock);

    iter = omap_iter(backups->omap);
    for (omap_each(iter, ti_backup_t, backup))
    {
        ti_val_t * mpinfo = ti_backup_as_mpval(backup);
        if (!mpinfo)
        {
            ti_val_unsafe_drop((ti_val_t *) varr);
            varr = NULL;
            goto stop;
        }
        VEC_push(varr->vec, mpinfo);
    }

stop:
    uv_mutex_unlock(backups->lock);
    return varr;
}

_Bool ti_backups_ok(void)
{
    omap_iter_t iter;
    _Bool backups_ok = true;

    uv_mutex_lock(backups->lock);

    iter = omap_iter(backups->omap);
    for (omap_each(iter, ti_backup_t, backup))
        if (backup->result_msg && backup->result_code != 0)
            backups_ok = false;

    uv_mutex_unlock(backups->lock);
    return backups_ok;
}

void ti_backups_del_backup(uint64_t backup_id, _Bool delete_files, ex_t * e)
{
    ti_backup_t * backup;
    uv_mutex_lock(backups->lock);

    backup = omap_rm(backups->omap, backup_id);
    if (backup)
    {
        if (delete_files)
            backups__delete_files(backup);

        ti_backup_destroy(backup);
        backups->changed = true;
    }
    else
        ex_set(e, EX_LOOKUP_ERROR,
                "backup with id %"PRIu64" not found", backup_id);

    uv_mutex_unlock(backups->lock);
}

_Bool ti_backups_has_backup(uint64_t backup_id)
{
    _Bool has_backup;

    uv_mutex_lock(backups->lock);

    has_backup = !!omap_get(backups->omap, backup_id);

    uv_mutex_unlock(backups->lock);

    return has_backup;
}

ti_val_t * ti_backups_backup_as_mpval(uint64_t backup_id, ex_t * e)
{
    ti_val_t * mpinfo = NULL;
    ti_backup_t * backup;

    uv_mutex_lock(backups->lock);

    backup = omap_get(backups->omap, backup_id);
    if (backup)
    {
        mpinfo = ti_backup_as_mpval(backup);
        if (!mpinfo)
            ex_set_mem(e);
    }
    else
        ex_set(e, EX_LOOKUP_ERROR,
                "backup with id %"PRIu64" not found", backup_id);

    uv_mutex_unlock(backups->lock);
    return mpinfo;
}

ti_backup_t * ti_backups_new_backup(
        uint64_t id,
        const char * fn_template,
        size_t fn_templare_n,
        uint64_t timestamp,
        uint64_t repeat,
        uint64_t max_files,
        uint64_t created_at,
        queue_t * files)
{
    ti_backup_t * backup = ti_backup_create(
            id,
            fn_template,
            fn_templare_n,
            timestamp,
            repeat,
            max_files,
            created_at,
            files);
    if (!backup)
        return NULL;

    uv_mutex_lock(backups->lock);

    if (omap_add(backups->omap, id, backup) != OMAP_SUCCESS)
    {
        ti_backup_destroy(backup);
        backup = NULL;
    }
    else
    {
        if (id >= backups->next_id)
            backups->next_id = id + 1;
        backups->changed = true;
    }
    uv_mutex_unlock(backups->lock);
    return backup;
}
