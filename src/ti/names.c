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


int ti_names_create(void)
{
    names = ti.names = smap_create();
    return -(names == NULL);
}

void ti_names_destroy(void)
{
    if (!names)
        return;
    smap_destroy(names, free);
    names = ti.names = NULL;
}

/*
 * non-critical function, just to inject some common names
 */
void ti_names_inject_common(void)
{
    (void) ti_names_from_str("_");
    (void) ti_names_from_str("a");
    (void) ti_names_from_str("b");
    (void) ti_names_from_str("c");
    (void) ti_names_from_str("x");
    (void) ti_names_from_str("y");
    (void) ti_names_from_str("z");
    (void) ti_names_from_str("tmp");
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

