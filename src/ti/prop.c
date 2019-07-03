/*
 * ti/prop.c
 */
#include <stdlib.h>
#include <ti.h>
#include <ti/prop.h>

ti_prop_t * ti_prop_create(ti_name_t * name, ti_val_t * val)
{
    ti_prop_t * prop = malloc(sizeof(ti_prop_t));
    if (!prop)
        return NULL;

    prop->val = val;
    prop->name = name;

    return prop;
}

void ti_prop_destroy(ti_prop_t * prop)
{
    if (!prop)
        return;
    ti_name_drop(prop->name);
    ti_val_drop(prop->val);
    free(prop);
}
