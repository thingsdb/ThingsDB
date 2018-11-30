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
    size_t events_in_achive;        /* number of events in archive,
                                       either on disk or in queue */
    size_t last_on_disk;            /* last event id written on disk */
    char * path;
    char * nodes_cevid_fn;          /* this file contains the last cevid
                                       by ALL nodes, saved at cleanup */
    queue_t * queue;                /* ordered ti_epkg_t on event_id */
};

int ti_archive_create(void);
void ti_archive_destroy(void);
int ti_archive_write_nodes_cevid(void);
ti_epkg_t * ti_archive_epkg_from_pkg(ti_pkg_t * pkg);
int ti_archive_load(void);
int ti_archive_push(ti_epkg_t * epkg);
int ti_archive_to_disk(void);
void ti_archive_cleanup(void);


#endif /* TI_ARCHIVE_H_ */
