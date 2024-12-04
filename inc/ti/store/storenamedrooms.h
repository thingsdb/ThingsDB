/*
 * ti/store/storenamedrooms.h
 */
#ifndef TI_STORE_NAMED_ROOMS_H_
#define TI_STORE_NAMED_ROOMS_H_

#include <util/smap.h>
#include <ti/collection.h>

int ti_store_named_rooms_store(smap_t * named_rooms, const char * fn);
int ti_store_named_rooms_restore(const char * fn, ti_collection_t * collection);

#endif /* TI_STORE_NAMED_ROOMS_H_ */


