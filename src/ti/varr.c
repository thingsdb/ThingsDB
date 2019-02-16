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
    varr->tp = TI_VAL_ARRAY;

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

    if (arr->tp == TI_VAL_TUPLE)
    {
        ti_incref(arr);
        return arr;
    }

    tuple = malloc(sizeof(ti_varr_t));
    if (!tuple)
        return NULL;

    tuple->ref = 1;
    tuple->tp = TI_VAL_TUPLE;
    tuple->tuple_with_things = arr->tp == TI_VAL_LIST;
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
    vec_destroy(varr->vec, (vec_destroy_cb) ti_val_drop);
}


/*
 * The destination array should have enough space to hold the new value.
 */
int ti_varr_append(ti_varr_t * to, ti_val_t * val, ex_t * e)
{
    assert (to->tp == TI_VAL_ARRAY || to->tp == TI_VAL_LIST);
    assert (vec_space(to->vec));

    if

    if (val->tp == TI_VAL_ARRAY || val->tp == TI_VAL_LIST)
    {
        ti_varr_t * tuple = ti_varr_to_tuple((ti_varr_t *) val);
        if (!tuple)
        {
            ex_set_alloc(e);
            return e->nr;
        }

        /* if this
        if (to->tp == TI_VAL_ARRAY && val->tp == TI_VAL_LIST)
            to->tp = TI_VAL_LIST;

        val = tuple;
    }
    else
    {
        ti_incref(val);
    }



    VEC_push(to->vec, val);

    if (to->tp == TI_VAL_ARRAY && )

    if (val->tp == TI_VAL_THINGS)
    {
        if (val->via.things->n)
        {
            ex_set(e, EX_BAD_DATA,
                    "type `%s` cannot be nested into into another array",
                    ti_val_str(val));
            return e->nr;
        }
        val->tp = TI_VAL_TUPLE;
    }

    if (val->tp == TI_VAL_THING)
    {
        /* TODO: I think we can convert back, not sure why I first thought
         * this was not possible. Maybe because of nesting? but that is
         * solved because nested are tuple and therefore not mutable */
        if (to_arr->tp == TI_VAL_ARRAY  && !to_arr->via.array->n)
            to_arr->tp = TI_VAL_THINGS;

        if (to_arr->tp == TI_VAL_ARRAY)
        {
            ex_set(e, EX_BAD_DATA,
                "type `%s` cannot be added into an array with other types",
                ti_val_str(val));
            return e->nr;
        }

        VEC_push(to_arr->via.things, val->via.thing);
        ti_val_weak_destroy(val);
        return 0;
    }

    if (to_arr->tp == TI_VAL_THINGS  && !to_arr->via.things->n)
        to_arr->tp = TI_VAL_ARRAY;

    if (to_arr->tp == TI_VAL_THINGS)
    {
        ex_set(e, EX_BAD_DATA,
            "type `%s` cannot be added into an array with `things`",
            ti_val_str(val));
        return e->nr;
    }

    if (ti_val_check_assignable(val, true, e))
        return e->nr;

    VEC_push(to_arr->via.array, val);
    return 0;
}
