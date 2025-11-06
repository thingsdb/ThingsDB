/*
 * ti/store/storecommits.h
 */
#ifndef TI_STORE_COMMITS_H_
#define TI_STORE_COMMITS_H_

#include <util/vec.h>
#include <ti/collection.h>

int ti_store_commits_store(vec_t * commits, const char * fn);
int ti_store_commits_restore(vec_t ** commits, const char * fn);

#endif /* TI_STORE_COMMITS_H_ */
