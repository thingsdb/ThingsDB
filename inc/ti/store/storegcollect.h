/*
 * ti/store/storegcollect.h
 */
#ifndef TI_STORE_GCOLLECT_H_
#define TI_STORE_GCOLLECT_H_

#include <ti/collection.h>
#include <util/queue.h>
#include <util/imap.h>

int ti_store_gcollect_store(queue_t * gc, const char * fn);
int ti_store_gcollect_store_data(queue_t * gc, const char * fn);
int ti_store_gcollect_restore(ti_collection_t * collection, const char * fn);
int ti_store_gcollect_restore_data(
        ti_collection_t * collection,
        imap_t * names,
        const char * fn);

#endif /* TI_STORE_GCOLLECT_H_ */
