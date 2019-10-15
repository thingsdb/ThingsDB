/*
 * ti/backups.h
 */
#include <stdlib.h>

#include <ti/backups.h>
#include <util/util.h>

static ti_backups_t * backups;
static ti_backups_t backups_;


int ti_backups_create(void)
{
    backups = &backups_;

    backups->lock =  malloc(sizeof(uv_mutex_t));
    backups->omap = omap_create();

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
    ti()->backups = NULL;
}

static ti_backup_t * backups__get_pending(uint64_t ts)
{
    omap_iter_t iter = omap_iter(backups->omap);
    for (omap_each(iter, ti_backup_t, backup))
        if (backup->timestamp < ts)
            return backup;
    return NULL;
}

static int backup__backup(const char * fn)
{

    sprintf(buff,"tar -czf %s -C %s", fn, ti()->cfg->storage_path);
    system(buff);

}


int ti_backups_backup(void)
{
    ti_backup_t * backup;
    uint64_t now = util_now_tsec();
    uv_mutex_lock(backups->lock);



    uv_mutex_unlock(backups->lock);
}
