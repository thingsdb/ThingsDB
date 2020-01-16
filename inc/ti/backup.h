/*
 * ti/backup.h
 */
#ifndef TI_BACKUP_H_
#define TI_BACKUP_H_

typedef struct ti_backup_s ti_backup_t;

#include <stdbool.h>
#include <inttypes.h>
#include <stddef.h>
#include <util/mpack.h>
#include <ti/val.h>


ti_backup_t * ti_backup_create(
        uint64_t id,
        const char * fn_template,
        size_t fn_templare_n,
        uint64_t timestamp,
        uint64_t repeat,
        uint64_t created_at);
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
    _Bool scheduled;
    int result_code;        /* last status code (0 = OK) */
};


#endif  /* TI_BACKUP_H_ */
