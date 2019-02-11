/*
 * util/query.c
 */
#include <ti/name.h>
#include <ti/arrow.h>
#include <langdef/nd.h>
#include <util/omap.h>
#include <util/query.h>

int query_rval_clear(ti_query_t * query)
{
    if (query->rval)
    {
        ti_val_clear(query->rval);
        return 0;
    }
    query->rval = ti_val_create(TI_VAL_NIL, NULL);
    return query->rval ? 0 : -1;
}

void query_rval_destroy(ti_query_t * query)
{
    ti_val_destroy(query->rval);
    query->rval = NULL;
}

void query_rval_weak_destroy(ti_query_t * query)
{
    ti_val_weak_destroy(query->rval);
    query->rval = NULL;
}

ti_prop_t * query_get_tmp_prop(ti_query_t * query, ti_name_t * name)
{
    if (!query->tmpvars)
        return NULL;
    for (vec_each(query->tmpvars, ti_prop_t, tmp))
        if (tmp->name == name)
            return tmp;
    return NULL;
}

