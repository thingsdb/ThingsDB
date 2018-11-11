/*
 * prop.c
 */
#include <stdlib.h>
#include <ti.h>
#include <ti/prop.h>

ti_prop_t * ti_prop_create(ti_name_t * name, ti_val_enum tp, void * v)
{
    ti_prop_t * prop = malloc(sizeof(ti_prop_t));
    if (!prop || ti_val_set(&prop->val, tp, v))
    {
        ti_prop_destroy(prop);
        return NULL;
    }
    prop->name = name;
    return prop;
}

ti_prop_t * ti_prop_createv(ti_name_t * name, ti_val_t * val)
{
    ti_prop_t * prop = malloc(sizeof(ti_prop_t));
    if (!prop || ti_val_copy(&prop->val, val))
    {
        ti_prop_destroy(prop);
        return NULL;
    }
    prop->name = name;
    return prop;
}

ti_prop_t * ti_prop_weak_create(ti_name_t * name, ti_val_enum tp, void * v)
{
    ti_prop_t * prop = malloc(sizeof(ti_prop_t));
    if (!prop)
        return NULL;
    ti_val_weak_set(&prop->val, tp, v);
    prop->name = name;
    return prop;
}

ti_prop_t * ti_prop_weak_createv(ti_name_t * name, ti_val_t * val)
{
    ti_prop_t * prop = malloc(sizeof(ti_prop_t));
    if (!prop)
        return NULL;
    ti_val_weak_copy(&prop->val, val);
    prop->name = name;
    return prop;
}

void ti_prop_destroy(ti_prop_t * prop)
{
    if (!prop)
        return;
    ti_name_drop(prop->name);
    ti_val_clear(&prop->val);
    free(prop);
}

void ti_prop_weak_destroy(ti_prop_t * prop)
{
    if (!prop)
        return;
    ti_name_drop(prop->name);
    free(prop);
}
