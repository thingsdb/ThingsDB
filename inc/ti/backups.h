/*
 * ti/backups.h
 */
#ifndef TI_BACKUPS_H_
#define TI_BACKUPS_H_

typedef struct ti_backups_s ti_backups_t;

#include <ex.h>
#include <ti/backup.h>
#include <ti/val.t.h>
#include <ti/varr.t.h>
#include <util/omap.h>
#include <util/queue.h>
#include <uv.h>

int ti_backups_create(void);
void ti_backups_destroy(void);
int ti_backups_backup(void);
int ti_backups_rm(void);
int ti_backups_restore(void);
int ti_backups_store(void);
size_t ti_backups_scheduled(void);
size_t ti_backups_pending(void);
_Bool ti_backups_require_away(void);
ti_varr_t * ti_backups_info(void);
void ti_backups_del_backup(uint64_t backup_id, _Bool delete_files, ex_t * e);
_Bool ti_backups_has_backup(uint64_t backup_id);
ti_val_t * ti_backups_backup_as_mpval(uint64_t backup_id, ex_t * e);
ti_backup_t * ti_backups_new_backup(
        uint64_t id,
        const char * fn_template,
        size_t fn_templare_n,
        uint64_t timestamp,
        uint64_t repeat,
        uint64_t max_files,
        uint64_t created_at,
        queue_t * files);


struct ti_backups_s
{
    omap_t * omap;
    uv_mutex_t * lock;
    char * fn;
    uint64_t next_id;
    _Bool changed;
};

#endif  /* TI_BACKUPS_H_ */
