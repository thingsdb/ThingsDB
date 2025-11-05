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
_Bool ti_names_no_ref(void);
void ti_names_destroy(void);
void ti_names_inject_common(void);
ti_name_t * ti_names_new(const char * str, size_t n);

/*
 * returns a name when the name exists and with a borrowed reference, if the
 * name does not exists, NULL will be the return value.
 */
static inline ti_name_t * ti_names_weak_get_strn(const char * str, size_t n)
{
    return smap_getn(names, str, n);
}

static inline ti_name_t * ti_names_get(const char * str, size_t n)
{
    ti_name_t * name = smap_getn(names, str, n);
    if (name)
    {
        ti_incref(name);
        return name;
    }
    return ti_names_new(str, n);
}

ti_name_t * ti_names_get_slow(const char * str, size_t n);

#define ti_names_from_raw(raw__) \
    ti_names_get((const char *) (raw__)->data, (raw__)->n)
#define ti_names_from_str_slow(str__) ti_names_get_slow((str__), strlen(str__))

static inline ti_name_t * ti_names_weak_from_raw(ti_raw_t * raw)
{
    return smap_getn(names, (const char *) raw->data, raw->n);
}

static inline ti_name_t * ti_names_weak_get_str(const char * str)
{
    return smap_get(names, str);
}


#endif /* TI_NAMES_H_ */
