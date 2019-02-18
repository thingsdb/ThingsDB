/*
 * names.h
 */
#ifndef TI_NAMES_H_
#define TI_NAMES_H_

#include <stdint.h>
#include <ti/name.h>

int ti_names_create(void);
void ti_names_destroy(void);
void ti_names_inject_common(void);
ti_name_t * ti_names_get(const char * str, size_t n);
ti_name_t * ti_names_weak_get(const char * str, size_t n);

#endif /* TI_NAMES_H_ */
