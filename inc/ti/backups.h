/*
 * ti/backups.h
 */
#ifndef TI_BACKUPS_H_
#define TI_BACKUPS_H_


#include <uv.h>
#include <ti/varr.h>
#include <ti/backup.h>
#include <util/omap.h>

typedef struct ti_backups_s ti_backups_t;

int ti_backups_create(void);
void ti_backups_destroy(void);
int ti_backups_backup(void);
size_t ti_backups_scheduled(void);
ti_varr_t * ti_backups_info(void);
uint64_t ti_backups_next_id(void);
ti_backup_t * ti_backups_new_backup(
        uint64_t id,
        const char * fn_template,
        size_t fn_templare_n,
        uint64_t timestamp,
        uint64_t repeat);


struct ti_backups_s
{
    omap_t * omap;
    uv_mutex_t * lock;
    _Bool changed;
};

#endif  /* TI_BACKUPS_H_ */
