/*
 * names.h
 */
#include <stdlib.h>
#include <string.h>
#include <ti.h>
#include <ti/names.h>
#include <ti/val.h>
#include <util/logger.h>
#include <util/smap.h>
#include <util/vec.h>

static smap_t * names;

int ti_names_create(void)
{
    names = ti()->names = smap_create();
    return -(names == NULL);
}

void ti_names_destroy(void)
{
    if (!names)
        return;
    smap_destroy(names, free);
    names = ti()->names = NULL;
}

/*
 * returns a name with a new reference or NULL in case of an error
 */
ti_name_t * ti_names_get(const char * str, size_t n)
{
    ti_name_t * name = smap_getn(names, str, n);
    if (name)
        return ti_grab(name);

    int rc;
    name = ti_name_create(str, n);
    if (!name || (rc = smap_add(names, name->str, name)))
    {
        ti_name_destroy(name);
        return NULL;
    }
    return name;
}

/*
 * returns a name when the name exists and with a borrowed reference, if the
 * name does not exists, NULL will be the return value.
 */
ti_name_t * ti_names_weak_get(const char * str, size_t n)
{
    return smap_getn(names, str, n);
}



