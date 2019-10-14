/*
 * ti/backup.h
 */
#ifndef TI_BACKUP_H_
#define TI_BACKUP_H_


typedef struct ti_backup_s ti_backup_t;

struct ti_backup_s
{
    uint64_t id;
    const char * fn;      /* with support for {EVENT_ID} {DATE} {TIME} {TIMESTAMP} */
    const char * last_msg;
    uint8_t status;
    uint64_t timestamp;
    uint64_t repeat;
};



#endif  /* TI_BACKUP_H_ */
