/*
 * ti/store/procedures.h
 */
#ifndef TI_STORE_PROCEDURES_H_
#define TI_STORE_PROCEDURES_H_

#include <util/vec.h>

int ti_store_procedures_store(const vec_t * procedures, const char * fn);
int ti_store_procedures_restore(
        vec_t ** procedures,
        const char * fn,
        imap_t * things);

#endif /* TI_STORE_PROCEDURES_H_ */
