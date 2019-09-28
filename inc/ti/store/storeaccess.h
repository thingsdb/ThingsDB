/*
 * ti/store/access.h
 */
#ifndef TI_STORE_ACCESS_H_
#define TI_STORE_ACCESS_H_

#include <util/vec.h>

int ti_store_access_store(const vec_t * access, const char * fn);
int ti_store_access_restore(vec_t ** access, const char * fn);

#endif /* TI_STORE_ACCESS_H_ */
