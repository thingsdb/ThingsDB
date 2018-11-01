/*
 * names.h
 */
#ifndef TI_NAMES_H_
#define TI_NAMES_H_

#include <stdint.h>
#include <ti/name.h>
#include <util/imap.h>

int ti_names_create(void);
void ti_names_destroy(void);
ti_name_t * ti_names_get(const char * str, size_t n);
ti_name_t * ti_names_weak_get(const char * str, size_t n);
int ti_names_store(const char * fn);
imap_t * ti_names_restore(const char * fn);

#endif /* TI_NAMES_H_ */
