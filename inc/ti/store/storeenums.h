/*
 * ti/store/storeenums.h
 */
#ifndef TI_STORE_ENUMS_H_
#define TI_STORE_ENUMS_H_

#include <ti/enums.h>

int ti_store_enums_store(ti_enums_t * enums, const char * fn);
int ti_store_enums_restore(ti_enums_t * enums, imap_t * names, const char * fn);
int ti_store_enums_restore_members(
        ti_enums_t * enums,
        imap_t * names,
        const char * fn);

#endif /* TI_STORE_ENUMS_H_ */
