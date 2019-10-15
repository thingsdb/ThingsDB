/*
 * ti/backups.h
 */
#include <stdlib.h>

#include <ti/backups.h>

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


int ti_backups_backup(void)
{
    omap_iter_t iter =
}
