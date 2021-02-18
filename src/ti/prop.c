/*
 * ti/prop.c
 */
#include <stdlib.h>
#include <ti.h>
#include <ti/name.h>
#include <ti/prop.h>
#include <ti/val.inline.h>

ti_prop_t * ti_prop_create(ti_name_t * name, ti_val_t * val)
{
    ti_prop_t * prop = malloc(sizeof(ti_prop_t));
    if (!prop)
        return NULL;

    prop->val = val;
    prop->name = name;

    return prop;
}

/*
 * Returns a duplicate of the prop with a new reference for the name and value.
 */
ti_prop_t * ti_prop_dup(ti_prop_t * prop)
{
    ti_prop_t * dup = malloc(sizeof(ti_prop_t));
    if (!prop)
        return NULL;

    memcpy(dup, prop, sizeof(ti_prop_t));
    ti_incref(dup->name);
    ti_incref(dup->val);

    return dup;
}

void ti_prop_destroy(ti_prop_t * prop)
{
    if (!prop)
        return;
    ti_name_unsafe_drop(prop->name);
    ti_val_unassign_drop(prop->val);
    free(prop);
}
