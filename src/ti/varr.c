/*
 * ti/varr.c
 */
#include <assert.h>
#include <tiinc.h>
#include <stdlib.h>
#include <ti/varr.h>
#include <ti/val.h>
#include <util/logger.h>


ti_varr_t * ti_varr_create(size_t sz)
{
    ti_varr_t * varr = malloc(sizeof(ti_varr_t));
    if (!varr)
        return NULL;

    varr->ref = 1;
    varr->tp = TI_VAL_ARR;
    varr->flags = 0;

    varr->vec = vec_new(sz);
    if (!varr->vec)
    {
        free(varr);
        return NULL;
    }

    return varr;
}

int ti_varr_to_tuple(ti_varr_t ** varr)
{
    ti_varr_t * tuple = *varr;

    if (tuple->flags & TI_ARR_FLAG_TUPLE)
        return 0;

    if (tuple->ref == 1)
    {
        tuple->flags |= TI_ARR_FLAG_TUPLE;
        return 0;
    }

    tuple = malloc(sizeof(ti_varr_t));
    if (!tuple)
        return -1;

    tuple->ref = 1;
    tuple->flags |= TI_ARR_FLAG_TUPLE;
    tuple->vec = vec_dup((*varr)->vec);

    if (!tuple->vec)
    {
        free(tuple);
        return -1;
    }

    for (vec_each(tuple->vec, ti_val_t, val))
        ti_incref(val);

    assert ((*varr)->ref > 1);
    ti_decref(*varr);
    *varr = tuple;
    return 0;
}

void ti_varr_destroy(ti_varr_t * varr)
{
    if (!varr)
        return;
    vec_destroy(varr->vec, (vec_destroy_cb) ti_val_drop);
    free(varr);
}

/*
 * does not increment `*v` reference counter but the value might change to
 * a (new) tuple.
 */
int ti_varr_append(ti_varr_t * to, void ** v, ex_t * e)
{
    assert (ti_varr_is_list(to));
    ti_val_t * val = *v;

    if (ti_val_make_assignable(val, e))
        return e->nr;

    switch (val->tp)
    {
    case TI_VAL_ARR:
        if (ti_varr_is_list((ti_varr_t *) val))
        {
            if (ti_varr_to_tuple((ti_varr_t **) v))
            {
                ex_set_alloc(e);
                return e->nr;
            }
            val = *v;
        }
        to->flags |= ((ti_varr_t *) val)->flags & TI_ARR_FLAG_THINGS;
        break;
    case TI_VAL_THING:
        to->flags |= TI_ARR_FLAG_THINGS;
        break;
    }

    if (vec_push(&to->vec, val))
        ex_set_alloc(e);

    return e->nr;
}

_Bool ti_varr_has_things(ti_varr_t * varr)
{
    if (ti_varr_may_have_things(varr))
        for (vec_each(varr->vec, ti_val_t, val))
            if (val->tp == TI_VAL_THING)
                return true;
    return false;
}

