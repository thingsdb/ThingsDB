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

    kind->name = strdup(name);
    kind->ref = 1;
    kind->flags = 0;
    kind->props = vec_create(0);

    if (!kind->props || !kind->name)
    {
        free(kind->props);
        free(kind->name);
        return NULL;
    }

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
        vec_destroy(kind->props, (vec_destroy_cb) rql_prop_destroy);
        free(kind->name);
        free(kind);
    }
}

/*
 * Append n props to kind. Make sure to update all existing elements
 * which are using this kind.
 *
 * All props will be automatically destroyed when the kind is destroyed.
 *
 * Returns 0 when successful or -1 in case of an allocation error.
 */
int rql_kind_append_props(rql_kind_t * kind, rql_prop_t * props[], uint32_t n)
{
    vec_t * tmp = vec_extend(kind->props, (void *) props, n);
    if (!tmp) return -1;

    kind->props = tmp;
    return 0;
}
