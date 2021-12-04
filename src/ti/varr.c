/*
 * ti/varr.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/closure.h>
#include <ti/opr.h>
#include <ti/spec.inline.h>
#include <ti/val.h>
#include <ti/varr.h>
#include <ti/vset.h>
#include <tiinc.h>
#include <util/logger.h>

int ti_varr_to_tuple(ti_varr_t ** varr)
{
    ti_varr_t * tuple = *varr;

    if (tuple->flags & TI_VARR_FLAG_TUPLE)
        return 0;  /* tuples cannot change so we do not require a copy */

    if (tuple->ref == 1)
    {
        tuple->flags |= TI_VARR_FLAG_TUPLE;
        tuple->parent = NULL;  /* make sure no parent is set */
        return 0;  /* with only one reference we do not require a copy */
    }

    tuple = malloc(sizeof(ti_tuple_t));
    if (!tuple)
        return -1;

    tuple->ref = 1;
    tuple->tp = TI_VAL_ARR;
    tuple->flags = TI_VARR_FLAG_TUPLE | ti_varr_may_flags(*varr);
    tuple->vec = vec_dup((*varr)->vec);
    /*
     * Note that `tuple` is allocation as a tuple but is casted as type `varr`
     * so do NOT set the `parent` and `name` since there is no room for
     * allocated!
     */

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

ti_varr_t * ti_varr_create(size_t sz)
{
    ti_varr_t * varr = malloc(sizeof(ti_varr_t));
    if (!varr)
        return NULL;

    varr->ref = 1;
    varr->tp = TI_VAL_ARR;
    varr->flags = 0;
    varr->parent = NULL;

    varr->vec = vec_new(sz);
    if (!varr->vec)
    {
        free(varr);
        return NULL;
    }

    return varr;
}

ti_varr_t * ti_tuple_from_vec(vec_t * vec)
{
    ti_varr_t * varr = malloc(sizeof(ti_varr_t));
    if (!varr)
        return NULL;

    varr->ref = 1;
    varr->tp = TI_VAL_ARR;
    varr->flags = \
            TI_VARR_FLAG_TUPLE|(vec->n?(TI_VARR_FLAG_MHT|TI_VARR_FLAG_MHR):0);
    varr->vec = vec;
    varr->parent = NULL;
    return varr;
}

/*
 * Should only be used when it is `ti_val_to_arr()` should succeed in normal
 * conditions.
 */
ti_varr_t * ti_varr_from_vec(vec_t * vec)
{
    ex_t e = {0};
    ti_varr_t * varr = malloc(sizeof(ti_varr_t));
    if (!varr)
        return NULL;

    varr->ref = 1;
    varr->tp = TI_VAL_ARR;
    varr->flags = 0;
    varr->vec = vec;
    varr->parent = NULL;
    for (vec_each_addr(vec, ti_val_t, v))
    {
        if (ti_val_to_arr(v, varr, &e))
        {
            log_critical(e.msg);
            free(varr);
            return NULL;
        }
    }
    return varr;
}

ti_varr_t * ti_varr_from_slice(
        ti_varr_t * source,
        ssize_t start,
        ssize_t stop,
        ssize_t step)
{
    ssize_t n = stop - start;
    uint32_t sz;
    ti_varr_t * varr = malloc(sizeof(ti_varr_t));
    if (!varr)
        return NULL;

    varr->ref = 1;
    varr->tp = TI_VAL_ARR;
    varr->flags = 0;
    varr->parent = NULL;

    /*
     * Calculate the exact size for the new list. This has to be exact since
     * the size is used below to fill the list with values.
     */
    n = n / step + !!(n % step);
    sz = (uint32_t) (n < 0 ? 0 : n);

    varr->vec = vec_new(sz);
    if (!varr->vec)
    {
        free(varr);
        return NULL;
    }

    for (n = start; sz--; n += step)
    {
        ti_val_t * val = VEC_get(source->vec, n);
        ti_incref(val);
        VEC_push(varr->vec, val);
    }

    return varr;
}

void ti_varr_destroy(ti_varr_t * varr)
{
    if (!varr)
        return;
    vec_destroy(varr->vec, (vec_destroy_cb) ti_val_unsafe_gc_drop);
    free(varr);
}

_Bool ti_varr_has_val(ti_varr_t * varr, ti_val_t * val)
{
    for (vec_each(varr->vec, ti_val_t, v))
        if (ti_opr_eq(v, val))
            return true;
    return false;
}

int ti_varr_nested_spec_err(ti_varr_t * varr, ti_val_t * val, ex_t * e)
{
    switch (ti_spec_check_nested_val(ti_varr_spec(varr), val))
    {
    case TI_SPEC_RVAL_SUCCESS:
        return 0;
    case TI_SPEC_RVAL_TYPE_ERROR:
        ex_set(e, EX_TYPE_ERROR,
                "type `%s` is not allowed in restricted array",
                ti_val_str(val));
        return e->nr;
    case TI_SPEC_RVAL_UTF8_ERROR:
        ex_set(e, EX_VALUE_ERROR,
                "array is restricted to UTF8 string values");
        return e->nr;
    case TI_SPEC_RVAL_UINT_ERROR:
        ex_set(e, EX_VALUE_ERROR,
                "array is restricted to integer values "
                "greater than or equal to 0");
        return e->nr;
    case TI_SPEC_RVAL_PINT_ERROR:
        ex_set(e, EX_VALUE_ERROR,
                "array is restricted to positive integer values");
        return e->nr;
    case TI_SPEC_RVAL_NINT_ERROR:
        ex_set(e, EX_VALUE_ERROR,
                "array is restricted to negative integer values");
        return e->nr;
    }
    assert(0);
    return 0;
}

int ti_varr_to_list(ti_varr_t ** varr)
{
    ti_varr_t * list = *varr;

    if (list->ref == 1)
        return 0;

    list = malloc(sizeof(ti_varr_t));
    if (!list)
        return -1;

    list->ref = 1;
    list->tp = TI_VAL_ARR;
    list->flags = ti_varr_may_flags(*varr);
    list->vec = vec_dup((*varr)->vec);
    list->parent = NULL;

    if (!list->vec)
    {
        free(list);
        return -1;
    }

    for (vec_each(list->vec, ti_val_t, val))
        ti_incref(val);

    ti_decref(*varr);
    *varr = list;

    return 0;
}

int ti_varr_copy(ti_varr_t ** varr, uint8_t deep)
{
    assert (deep);

    int rc = 0;
    ti_varr_t * list = malloc(sizeof(ti_varr_t));
    if (!list)
        return -1;

    list->ref = 1;
    list->tp = TI_VAL_ARR;
    list->flags = ti_varr_may_flags(*varr);
    list->vec = vec_dup((*varr)->vec);
    list->parent = NULL;

    if (!list->vec)
    {
        free(list);
        return -1;
    }

    for (vec_each_addr(list->vec, ti_val_t, val))
    {
        ti_incref(*val);
        rc = rc || ti_val_copy(val, NULL, NULL, deep);
    }

    if (rc)
    {
        ti_varr_destroy(list);
        return -1;
    }

    ti_val_unsafe_drop((ti_val_t *) *varr);
    *varr = list;

    return 0;
}

int ti_varr_dup(ti_varr_t ** varr, uint8_t deep)
{
    assert (deep);

    int rc = 0;
    ti_varr_t * list = malloc(sizeof(ti_varr_t));
    if (!list)
        return -1;

    list->ref = 1;
    list->tp = TI_VAL_ARR;
    list->flags = ti_varr_may_flags(*varr);
    list->vec = vec_dup((*varr)->vec);
    list->parent = NULL;

    if (!list->vec)
    {
        free(list);
        return -1;
    }

    for (vec_each_addr(list->vec, ti_val_t, val))
    {
        ti_incref(*val);
        rc = rc || ti_val_dup(val, NULL, NULL, deep);
    }

    if (rc)
    {
        ti_varr_destroy(list);
        return -1;
    }

    ti_val_unsafe_drop((ti_val_t *) *varr);
    *varr = list;

    return 0;
}


/*
 * Do not use this method, but the in-line method ti_varr_eq() instead since
 * this functions already takes the assumption that `a` and `b` are different
 * arrays but of equal size.
 */
_Bool ti__varr_eq(ti_varr_t * varra, ti_varr_t * varrb)
{
    size_t i = 0;

    assert (varra != varrb && varra->vec->n == varrb->vec->n);

    for (vec_each(varra->vec, ti_val_t, va), ++i)
        if (!ti_opr_eq(va, VEC_get(varrb->vec, i)))
            return false;

    return true;
}

