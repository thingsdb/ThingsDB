/*
 * names.h
 */
#ifndef TI_NAMES_H_
#define TI_NAMES_H_

#include <stdint.h>
#include <tiinc.h>
#include <ti/name.h>
#include <ti/raw.h>
#include <util/smap.h>

extern smap_t * names;

int ti_names_create(void);
void ti_names_destroy(void);
void ti_names_inject_common(void);
ti_name_t * ti_names_new(const char * str, size_t n);

/*
 * returns a name when the name exists and with a borrowed reference, if the
 * name does not exists, NULL will be the return value.
 */
static inline ti_name_t * ti_names_weak_get(const char * str, size_t n)
{
    return smap_getn(names, str, n);
}

TI_INLINE(ti_name_t *) ti_names_get(const char * str, size_t n)
{
    ti_name_t * name = smap_getn(names, str, n);
    if (name)
    {
        ti_incref(name);
        return name;
    }
    return ti_names_new(str, n);
}

static inline ti_name_t * ti_names_from_raw(ti_raw_t * raw)
{
    return ti_names_get((const char *) raw->data, raw->n);
}

static inline ti_name_t * ti_names_weak_from_raw(ti_raw_t * raw)
{
    return ti_names_weak_get((const char *) raw->data, raw->n);
}

static inline ti_name_t * ti_names_weak_get_str(const char * str)
{
    return smap_get(names, str);
}


#endif /* TI_NAMES_H_ */
