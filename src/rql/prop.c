/*
 * prop.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <rql/prop.h>

/*
 * Returns a prop of NULL in case of an allocation error.
 *
 * Argument via can be either NULL, a kind or value. When a kind a used, make
 * sure the reference is incremented. When destroying the prop, the reference
 * will be decremented.
 */
rql_prop_t * rql_prop_create(
        const char * name,
        rql_val_t tp,
        uint32_t flags,
        rql_kind_t * kind,
        rql_prop_t * fwd_prop,
        rql_val_via_t * def)
{
    rql_prop_t * prop = (rql_prop_t *) malloc(sizeof(rql_prop_t));

    if (!prop) return NULL;

    if (!(prop->name = strdup(name)))
    {
        free(prop);
        return NULL;
    }

    prop->tp = tp;
    prop->flags = flags;
    prop->kind = kind;
    prop->prop = fwd_prop;
    prop->def = def;

    return prop;
}

void rql_prop_destroy(rql_prop_t * prop)
{
    if (!prop) return;

    free(prop->name);

    if (prop->tp == RQL_VAL_ELEM && prop->kind)
    {
        rql_kind_drop(prop->kind);
    }
    else if (prop->tp != RQL_VAL_ELEM && prop->def)
    {
        assert (~prop->flags & RQL_PROP_IS_ARR);
        rql_val_destroy(prop->tp, prop->def);
    }

    free(prop);
}

