/*
 * ti/store/procedures.h
 */
#ifndef TI_STORE_PROCEDURES_H_
#define TI_STORE_PROCEDURES_H_

#include <util/vec.h>
#include <ti/collection.h>

int ti_store_procedures_store(smap_t * procedures, const char * fn);
int ti_store_procedures_restore(
        smap_t * procedures,
        const char * fn,
        ti_collection_t * collection);

#endif /* TI_STORE_PROCEDURES_H_ */
