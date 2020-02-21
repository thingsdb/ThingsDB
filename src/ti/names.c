/*
 * names.h
 */
#include <stdlib.h>
#include <string.h>
#include <ti.h>
#include <ti/names.h>
#include <ti/val.h>
#include <util/logger.h>
#include <util/vec.h>

smap_t * names;

#define NAMES__INJECT(__s) __s, strlen(__s)

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
 * non-critical function, just to inject some common names
 */
void ti_names_inject_common(void)
{
    (void) ti_names_get(NAMES__INJECT("_"));
    (void) ti_names_get(NAMES__INJECT("a"));
    (void) ti_names_get(NAMES__INJECT("b"));
    (void) ti_names_get(NAMES__INJECT("c"));
    (void) ti_names_get(NAMES__INJECT("x"));
    (void) ti_names_get(NAMES__INJECT("y"));
    (void) ti_names_get(NAMES__INJECT("z"));
    (void) ti_names_get(NAMES__INJECT("tmp"));
}

/*
 * returns a name with a new reference or NULL in case of an error
 */
ti_name_t * ti_names_new(const char * str, size_t n)
{
    ti_name_t * name = ti_name_create(str, n);
    if (!name || smap_add(names, name->str, name))
    {
        free(name);
        return NULL;
    }
    return name;
}

