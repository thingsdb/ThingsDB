/*
 * ti/backups.h
 */
#include <stdlib.h>

#include <ti/backups.h>
#include <ti/backup.h>
#include <ti.h>
#include <util/util.h>
#include <util/buf.h>
#include <util/fx.h>

static const char * backups__fn             = "node_backups.mp";


static ti_backups_t * backups;
static ti_backups_t backups_;

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

    ti()->backups = backups;
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
    ti()->backups = NULL;
}

/* this function requires a lock since it returns a `backup` which requires
 * a lock by itself;
 */
static ti_backup_t * backups__get_pending(uint64_t ts, uint64_t id)
{
    omap_iter_t iter = omap_iter(backups->omap);
    for (omap_each(iter, ti_backup_t, backup))
        if (backup->scheduled && backup->id >= id && backup->timestamp < ts)
            return backup;
    return NULL;
}


void ti_backups_upd_status(uint64_t backup_id, int rc, buf_t * buf)
{
    uint64_t now = util_now_tsec();
    ti_backup_t * backup;

    if (rc)
        log_error("backup result: %.*s", (int) buf->len, buf->data);
    else
        log_info("backup result: %.*s", (int) buf->len, buf->data);

    uv_mutex_lock(backups->lock);

    backup = omap_get(backups->omap, backup_id);

    if (!backup)
        goto done;

    free(backup->result_msg);
    backup->result_msg = strndup(buf->data, buf->len);

    backup->result_code = rc;

    if (backup->repeat)
    {
        do
            backup->timestamp += backup->repeat;
        while (backup->timestamp < now);
    }
    else
        backup->scheduled = false;

    backups->changed = true;

done:
    uv_mutex_unlock(backups->lock);
}

void backups__run(uint64_t backup_id, const char * job)
{
    char buffer[512];
    int rc = -1;
    FILE * fp;
    buf_t buf;
    buf_init(&buf);

    fp = popen(job, "r");
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

        if (rc == 0)
        {
            uint64_t now = util_now_tsec();
            struct tm * tm_info;
            tm_info = gmtime((const time_t *) &now);

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
        if (msgpack_pack_array(&pk, 8) ||
            msgpack_pack_uint64(&pk, backup->id) ||
            msgpack_pack_uint64(&pk, backup->created_at) ||
            msgpack_pack_uint64(&pk, backup->timestamp) ||
            msgpack_pack_uint64(&pk, backup->repeat) ||
            mp_pack_str(&pk, backup->fn_template) ||
            (backup->result_msg
                    ? mp_pack_str(&pk, result_msg)
                    : msgpack_pack_nil(&pk)) ||
            mp_pack_bool(&pk, backup->scheduled) ||
            msgpack_pack_fix_int32(&pk, backup->result_code)
        ) goto fail;
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

    backups->fn = fx_path_join(ti()->cfg->storage_path, backups__fn);
    return backups->fn ? 0 : -1;
}

int ti_backups_rm(void)
{
    if (backups__ensure_fn())
        return -1;

    if (fx_file_exist(ti_.backups->fn))
    {
        log_warning("removing backup schedules: `%s`", ti_.backups->fn);
        (void) unlink(ti_.backups->fn);
    }
    return 0;
}

int ti_backups_restore(void)
{
    int rc = -1;
    fx_mmap_t fmap;
    size_t i;
    mp_obj_t obj, mp_ver, mp_id, mp_ts, mp_repeat, mp_fn,
             mp_msg, mp_plan, mp_code, mp_created;
    mp_unp_t up;
    ti_backup_t * backup;
    uint64_t now = util_now_tsec();

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
        if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 8 ||
            mp_next(&up, &mp_id) != MP_U64 ||
            mp_next(&up, &mp_created) != MP_U64 ||
            mp_next(&up, &mp_ts) != MP_U64 ||
            mp_next(&up, &mp_repeat) != MP_U64 ||
            mp_next(&up, &mp_fn) != MP_STR ||
            mp_next(&up, &mp_msg) <= 0  ||
            mp_next(&up, &mp_plan) != MP_BOOL ||
            mp_next(&up, &mp_code) != MP_I64
        ) goto fail1;

        backup = ti_backups_new_backup(
                mp_id.via.u64,
                mp_fn.via.str.data,
                mp_fn.via.str.n,
                mp_ts.via.u64,
                mp_repeat.via.u64,
                mp_created.via.u64);

        if (backup)
        {
            backup->result_msg = mp_msg.tp == MP_STR
                    ? strndup(mp_msg.via.str.data, mp_msg.via.str.n)
                    : NULL;
            backup->result_code = (int) mp_code.via.i64;
            backup->scheduled = mp_plan.via.bool_;
            if (backup->repeat)
                while (backup->timestamp < now)
                    backup->timestamp += backup->repeat;
        }
    }

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
    uint64_t backup_id = 0;
    char * job = NULL;
    ti_backup_t * backup;
    uint64_t now = util_now_tsec();

    do
    {
        ti_sleep(100);

        uv_mutex_lock(backups->lock);

        backup = backups__get_pending(now, backup_id);
        if (backup)
        {
            backup_id = backup->id;
            job = ti_backup_job(backup);
        }

        uv_mutex_unlock(backups->lock);

        if (!job)
            break;

        backups__run(backup_id, job);
        free(job);
        job = NULL;
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
    uint64_t now = util_now_tsec();
    size_t n = 0;
    omap_iter_t iter;


    uv_mutex_lock(backups->lock);

    iter = omap_iter(backups->omap);
    for (omap_each(iter, ti_backup_t, backup))
        if (backup->scheduled && backup->timestamp < now)
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
            ti_val_drop((ti_val_t *) varr);
            varr = NULL;
            goto stop;
        }
        VEC_push(varr->vec, mpinfo);
    }

stop:
    uv_mutex_unlock(backups->lock);
    return varr;
}

void ti_backups_del_backup(uint64_t backup_id, ex_t * e)
{
    ti_backup_t * backup;
    uv_mutex_lock(backups->lock);

    backup = omap_rm(backups->omap, backup_id);
    if (backup)
    {
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
        uint64_t created_at)
{
    ti_backup_t * backup = ti_backup_create(
            id,
            fn_template,
            fn_templare_n,
            timestamp,
            repeat,
            created_at);
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
