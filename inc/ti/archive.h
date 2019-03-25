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
    size_t archived_on_disk;        /* number of events stored on disk since
                                       the last full store, the actual amount
                                       stored can be higher */
    uint64_t start_event_id;        /* archive starts with this event id,
                                       either on disk or in memory
                                    */
    uint64_t * sevid;               /* last event id written on disk, this
                                       value is also updated if a full store is
                                       done
                                    */
    char * path;
    char * nodes_scevid_fn;         /* this file contains the last cevid
                                       and sevid by ALL nodes, saved at
                                       cleanup
                                    */
    queue_t * queue;                /* ordered ti_epkg_t on event_id */
};

int ti_archive_create(void);
void ti_archive_destroy(void);
int ti_archive_write_nodes_scevid(void);
int ti_archive_init(void);
int ti_archive_load(void);
int ti_archive_push(ti_epkg_t * epkg);
int ti_archive_to_disk(void);
void ti_archive_cleanup(void);


#endif /* TI_ARCHIVE_H_ */
