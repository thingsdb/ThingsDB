/*
 * ti/varr.c
 */
#include <assert.h>
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

ti_varr_t * ti_varr_to_tuple(ti_varr_t * arr)
{
    ti_varr_t * tuple;

    if (arr->flags & TI_ARR_FLAG_TUPLE)
    {
        ti_incref(arr);
        return arr;
    }

    tuple = malloc(sizeof(ti_varr_t));
    if (!tuple)
        return NULL;

    tuple->ref = 1;
    tuple->flags |= TI_ARR_FLAG_TUPLE;
    tuple->vec = vec_dup(arr->vec);

    if (!tuple->vec)
    {
        free(tuple);
        return NULL;
    }

    for (vec_each(tuple->vec, ti_val_t, val))
        ti_incref(val);

    return tuple;
}


void ti_varr_destroy(ti_varr_t * varr)
{
    if (!varr)
        return;
    vec_destroy(varr->vec, (vec_destroy_cb) ti_val_drop);
    free(varr);
}

/*
 * Increments `val` reference counter when assigned to the array.
 */
int ti_varr_append(ti_varr_t * to, ti_val_t * val, ex_t * e)
{
    assert (ti_varr_is_list(to));

    if (ti_val_check_assignable(val, true, e))
        return e->nr;

    if (ti_val_is_list(val))
    {
        ti_varr_t * tuple = ti_varr_to_tuple((ti_varr_t *) val);
        if (!tuple)
        {
            ex_set_alloc(e);
            return e->nr;
        }

        to->flags |= tuple->flags & TI_ARR_FLAG_THINGS;
        val = (ti_val_t *) tuple;
    }
    else
    {
        ti_incref(val);
    }

    if (vec_push(&to->vec, val))
    {
        ti_decref(val);
        ex_set_alloc(e);
    }

    return e->nr;
}

