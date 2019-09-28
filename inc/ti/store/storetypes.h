/*
 * ti/store/things.h
 */
#ifndef TI_STORE_TYPES_H_
#define TI_STORE_TYPES_H_

#include <ti/types.h>

int ti_store_types_store(ti_types_t * types, const char * fn);
int ti_store_types_restore(ti_types_t * types, imap_t * names, const char * fn);

#endif /* TI_STORE_TYPES_H_ */
