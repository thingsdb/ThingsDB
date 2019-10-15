/*
 * ti/backups.h
 */
#include <stdlib.h>

#include <ti/backups.h>
#include <ti/backup.h>
#include <ti.h>
#include <util/util.h>
#include <util/buf.h>

static ti_backups_t * backups;
static ti_backups_t backups_;


int ti_backups_create(void)
{
    backups = &backups_;

    backups->lock =  malloc(sizeof(uv_mutex_t));
    backups->omap = omap_create();
    backups->changed = false;

    if (!backups->omap || !backups->lock || uv_mutex_init(backups->lock))
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
    ti()->backups = NULL;
}

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
    int rc;
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

int ti_backups_backup(void)
{
    uint64_t backup_id = 0;
    char * job = NULL;
    ti_backup_t * backup;
    uint64_t now = util_now_tsec();

    do
    {
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
        /* TODO: save */
    }

    return 0;
}

size_t ti_backups_scheduled(void)
{
    size_t n = 0;
    omap_iter_t iter = omap_iter(backups->omap);

    uv_mutex_lock(backups->lock);

    for (omap_each(iter, ti_backup_t, backup))
        if (backup->scheduled)
            ++n;

    uv_mutex_unlock(backups->lock);

    return n;
}

ti_varr_t * ti_backups_info(void)
{
    ti_varr_t * varr = ti_varr_create(backups->omap->n);
    if (!varr)
        return NULL;

    uv_mutex_lock(backups->lock);

    omap_iter_t iter = omap_iter(backups->omap);
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

uint64_t ti_backups_next_id(void)
{
    uint64_t id = 0;
    omap_iter_t iter = omap_iter(backups->omap);
    for (omap_each(iter, ti_backup_t, backup))
        id = omap_iter_id(iter) + 1;
    return id;
}

ti_backup_t * ti_backups_new_backup(
        uint64_t id,
        const char * fn_template,
        size_t fn_templare_n,
        uint64_t timestamp,
        uint64_t repeat)
{
    ti_backup_t * backup = ti_backup_create(
            id, fn_template, fn_templare_n, timestamp, repeat);
    if (!backup)
        return NULL;

    uv_mutex_lock(backups->lock);

    if (omap_add(backups->omap, id, backup) != OMAP_SUCCESS)
    {
        ti_backup_destroy(backup);
        backup = NULL;
    }
    else
        backups->changed = true;

    uv_mutex_unlock(backups->lock);
    return backup;
}
