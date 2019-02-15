/*
 * util/query.c
 */
#include <ti/name.h>
#include <ti/arrow.h>
#include <langdef/nd.h>
#include <util/omap.h>
#include <util/query.h>

ti_prop_t * query_get_tmp_prop(ti_query_t * query, ti_name_t * name)
{
    if (!query->tmpvars)
        return NULL;
    for (vec_each(query->tmpvars, ti_prop_t, tmp))
        if (tmp->name == name)
            return tmp;
    return NULL;
}

