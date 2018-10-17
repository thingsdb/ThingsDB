/*
 * archive.h
 */
#ifndef TI_ARCHIVE_H_
#define TI_ARCHIVE_H_

typedef struct ti_archive_s  ti_archive_t;

#include <stdint.h>
#include <ti/event.h>
#include <util/queue.h>

struct ti_archive_s
{
    uint64_t offset;
    queue_t * rawev;      /* ti_raw_t */
};

ti_archive_t * ti_archive_create(void);
void ti_archive_destroy(ti_archive_t * archive);
int ti_archive_event(ti_archive_t * archive, ti_event_t * event);

#endif /* TI_ARCHIVE_H_ */
