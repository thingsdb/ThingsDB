/*
 * names.h
 */
#ifndef TI_NAMES_H_
#define TI_NAMES_H_

#include <stdint.h>
#include <ti/name.h>
#include <util/smap.h>

extern smap_t * names;

int ti_names_create(void);
void ti_names_destroy(void);
void ti_names_inject_common(void);
ti_name_t * ti_names_get(const char * str, size_t n);
static inline ti_name_t * ti_names_weak_get(const char * str, size_t n);

/*
 * returns a name when the name exists and with a borrowed reference, if the
 * name does not exists, NULL will be the return value.
 */
static inline ti_name_t * ti_names_weak_get(const char * str, size_t n)
{
    return smap_getn(names, str, n);
}


#endif /* TI_NAMES_H_ */
