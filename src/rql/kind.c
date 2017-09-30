/*
 * kind.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <string.h>
#include <rql/kind.h>

/*
 * Returns a new kind or NULL in case of an allocation error.
 */
rql_kind_t * rql_kind_create(const char * name)
{
    rql_kind_t * kind = (rql_kind_t *) malloc(sizeof(rql_kind_t));

    if (!kind) return NULL;

    if (!(kind->name = strdup(name)))
    {
        free(kind);
        return NULL;
    }

    kind->ref = 1;
    kind->flags = 0;
    kind->props = 0;
    kind->props = NULL;

    return kind;
}

/*
 * Increments the reference counter and returns the kind.
 */
rql_kind_t * rql_kind_grab(rql_kind_t * kind)
{
    kind->ref++;
    return kind;
}

/*
 * Drops a kind when no references are left. When dropped, all assigned props
 * will be destroyed too.
 */
void rql_kind_drop(rql_kind_t * kind)
{
    if (!--kind->ref)
    {
        free(kind->name);
        for (uint32_t i = 0; i < kind->props_n; i++)
        {
            rql_prop_destroy(kind->props[i]);
        }
        free(kind->props);
        free(kind);
    }
}

/*
 * Append n props to kind. Make sure to update all existing elements
 * which are using this kind.
 *
 * All props will be automatically destroyed when the kind is destroyed.
 *
 * Returns 0 when successful.
 */
int rql_kind_append_props(rql_kind_t * kind, rql_prop_t * props[], uint32_t n)
{
    kind->props_n += n;

    rql_prop_t ** tmp = (rql_prop_t **) realloc(
            kind->props,
            kind->props_n * sizeof(rql_prop_t*));

    if (!tmp)
    {
        kind->props_n -= n;
        return -1;
    }

    kind->props = tmp;
    memcpy(kind->props + (kind->props_n - n), props, n * sizeof(rql_prop_t*));

    return 0;
}
