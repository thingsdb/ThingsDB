/*
 * ti/backup.h
 */
#include <ti/backup.h>



void ti_backup_destroy(ti_backup_t * backup)
{
    if (!backup)
        return;
    free(backup->fn);
    free(backup->last_msg);
    free(backup);
}


