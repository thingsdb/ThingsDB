/*
 * ti/archive.h
 */
#ifndef TI_ARCHIVE_H_
#define TI_ARCHIVE_H_

typedef struct ti_archive_s  ti_archive_t;

#include <stdint.h>
#include <ti/cpkg.t.h>
#include <util/queue.h>
#include <util/vec.h>

struct ti_archive_s
{
    char * path;
    queue_t * queue;                /* ordered ti_cpkg_t on change_id */
    vec_t * archfiles;              /* ti_archfile_t, unordered */
};

int ti_archive_create(void);
void ti_archive_destroy(void);
int ti_archive_rmdir(void);
int ti_archive_init(void);
int ti_archive_load(void);
int ti_archive_push(ti_cpkg_t * cpkg);
int ti_archive_to_disk(void);
uint64_t ti_archive_get_first_change_id(void);
ti_cpkg_t * ti_archive_get_change(uint64_t change_id);


#endif /* TI_ARCHIVE_H_ */
