/*
 * ti/archive.h
 */
#ifndef TI_ARCHIVE_H_
#define TI_ARCHIVE_H_

typedef struct ti_archive_s  ti_archive_t;

#include <stdint.h>
#include <ti/archive.h>
#include <ti/epkg.h>
#include <util/queue.h>

struct ti_archive_s
{
    size_t events_on_disk;          /* number of events saved on disk */
    size_t last_on_disk;            /* last event id written on disk */
    char * path;
    queue_t * queue;                /* ti_rpkg_t */
};

int ti_archive_create(void);
void ti_archive_destroy(void);
int ti_archive_load(void);
int ti_archive_push(ti_epkg_t * epkg);
int ti_archive_to_disk(_Bool with_sleep);
void ti_archive_cleanup(_Bool with_sleep);


#endif /* TI_ARCHIVE_H_ */
