/*
 * ti/backup.h
 */
#ifndef TI_BACKUP_H_
#define TI_BACKUP_H_

/*
 * Default is 1 for non-repeat or TI_BACKUP_DEFAULT_MAX_FILES when it is a
 * repeating schedule.
 */
#define TI_BACKUP_DEFAULT_MAX_FILES 7

typedef struct ti_backup_s ti_backup_t;

#include <stdbool.h>
#include <inttypes.h>
#include <stddef.h>
#include <ti/val.t.h>
#include <ti/raw.t.h>
#include <util/mpack.h>
#include <util/queue.h>


ti_backup_t * ti_backup_create(
        uint64_t id,
        const char * fn_template,
        size_t fn_templare_n,
        uint64_t timestamp,
        uint64_t repeat,
        uint64_t max_files,
        uint64_t created_at,
        queue_t * files);
char * ti_backup_job(ti_backup_t * backup);
void ti_backup_destroy(ti_backup_t * backup);
int ti_backup_info_to_pk(ti_backup_t * backup, msgpack_packer * pk);
ti_val_t * ti_backup_as_mpval(ti_backup_t * backup);


struct ti_backup_s
{
    uint64_t id;
    uint64_t timestamp;     /* Next run, UNIX time-stamp in seconds */
    uint64_t repeat;        /* Repeat every X seconds */
    uint64_t created_at;    /* UNIX time-stamp in seconds */
    char * fn_template;     /* {EVENT} {DATE} {TIME} */
    char * result_msg;      /* last status message */
    ti_raw_t * work_fn;     /* current backup file name */
    queue_t * files;        /* ti_raw_t, successful files, size: >=max_files */
    size_t max_files;       /* max files queue size */
    _Bool scheduled;        /* true when the backup is scheduled to run */
    int result_code;        /* last status code (0 = OK) */
};


#endif  /* TI_BACKUP_H_ */
