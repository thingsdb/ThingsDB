/*
 * ti/backup.h
 */
#ifndef TI_BACKUP_H_
#define TI_BACKUP_H_


typedef struct ti_backup_s ti_backup_t;

void ti_backup_destroy(ti_backup_t * backup);

struct ti_backup_s
{
    uint64_t id;
    const char * fn_template;   /* {EVENT} {DATE} {TIME} */
    const char * last_msg;
    uint8_t status;
    uint8_t pad0_;
    uint16_t fn_size;
    uint64_t timestamp;
    uint64_t repeat;
};



#endif  /* TI_BACKUP_H_ */
