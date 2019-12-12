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

static int varr__to_tuple(ti_varr_t ** varr)
{
    ti_varr_t * tuple = *varr;

    if (tuple->flags & TI_VFLAG_ARR_TUPLE)
        return 0;  /* tuples cannot change so we do not require a copy */

    if (tuple->ref == 1)
    {
        tuple->flags |= TI_VFLAG_ARR_TUPLE;
        return 0;  /* with only one reference we do not require a copy */
    }

    tuple = malloc(sizeof(ti_varr_t));
    if (!tuple)
        return -1;

    tuple->ref = 1;
    tuple->tp = TI_VAL_ARR;
    tuple->flags = TI_VFLAG_ARR_TUPLE | ((*varr)->flags & TI_VFLAG_ARR_MHT);
    tuple->spec = (*varr)->spec;
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

ti_varr_t * ti_varr_create(size_t sz)
{
    ti_varr_t * varr = malloc(sizeof(ti_varr_t));
    if (!varr)
        return NULL;

    varr->ref = 1;
    varr->tp = TI_VAL_ARR;
    varr->flags = 0;
    varr->spec = TI_SPEC_ANY;

    varr->vec = vec_new(sz);
    if (!varr->vec)
    {
        free(varr);
        return NULL;
    }

    return varr;
}

ti_varr_t * ti_varr_from_vec(vec_t * vec)
{
    ti_varr_t * varr = malloc(sizeof(ti_varr_t));
    if (!varr)
        return NULL;

    varr->ref = 1;
    varr->tp = TI_VAL_ARR;
    varr->flags = 0;
    varr->spec = TI_SPEC_ANY;

    varr->vec = vec;
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
    varr->spec = source->spec;

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
        ti_val_t * val = vec_get(source->vec, n);
        ti_incref(val);
        VEC_push(varr->vec, val);
    }

    return varr;
}

void ti_varr_destroy(ti_varr_t * varr)
{
    if (!varr)
        return;
    vec_destroy(varr->vec, (vec_destroy_cb) ti_val_drop);
    free(varr);
}

int ti_varr_val_prepare(ti_varr_t * to, void ** v, ex_t * e)
{
    assert (ti_varr_is_list(to));  /* `to` must be a list */

    switch (ti_spec_check_val(to->spec, (ti_val_t *) *v))
    {
    case TI_SPEC_RVAL_SUCCESS:
        break;
    case TI_SPEC_RVAL_TYPE_ERROR:
        ex_set(e, EX_TYPE_ERROR,
                "type `%s` is not allowed in restricted array",
                ti_val_str((ti_val_t *) *v));
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

    switch (((ti_val_t *) *v)->tp)
    {
    case TI_VAL_SET:
        if (ti_vset_to_tuple((ti_vset_t **) v))
        {
            ex_set_mem(e);
            return e->nr;
        }
        to->flags |= ((ti_varr_t *) *v)->flags & TI_VFLAG_ARR_MHT;
        break;
    case TI_VAL_CLOSURE:
        if (ti_closure_unbound((ti_closure_t *) *v, e))
        {
            ex_set_mem(e);
            return e->nr;
        }
        break;
    case TI_VAL_ARR:
        if (ti_varr_is_list((ti_varr_t *) *v))
        {
            if (varr__to_tuple((ti_varr_t **) v))
            {
                ex_set_mem(e);
                return e->nr;
            }
        }
        to->flags |= ((ti_varr_t *) *v)->flags & TI_VFLAG_ARR_MHT;
        break;
    case TI_VAL_THING:
        to->flags |= TI_VFLAG_ARR_MHT;
        break;
    }
    return e->nr;
}

/*
 * does not increment `*v` reference counter but the value might change to
 * a (new) tuple pointer.
 */
int ti_varr_set(ti_varr_t * to, void ** v, size_t idx, ex_t * e)
{
    if (ti_varr_val_prepare(to, v, e))
        return e->nr;

    ti_val_drop((ti_val_t *) vec_get(to->vec, idx));
    to->vec->data[idx] = *v;
    return 0;
}

_Bool ti_varr_has_things(ti_varr_t * varr)
{
    if (ti_varr_may_have_things(varr))
    {
        for (vec_each(varr->vec, ti_val_t, val))
            if (val->tp == TI_VAL_THING)
                return true;

        /* Remove the flag since no `things` are found in the array */
        varr->flags &= ~TI_VFLAG_ARR_MHT;
    }
    return false;
}

int ti_varr_to_list(ti_varr_t ** varr)
{
    ti_varr_t * list = *varr;

    if (list->ref == 1)
    {
        /* This can never happen to a tuple since a tuple is always nested
         * and therefore always has more than one reference */
        assert (~list->flags & TI_VFLAG_ARR_TUPLE);
        return 0;
    }

    list = malloc(sizeof(ti_varr_t));
    if (!list)
        return -1;

    list->ref = 1;
    list->tp = TI_VAL_ARR;
    list->flags = (*varr)->flags & TI_VFLAG_ARR_MHT;
    list->spec = (*varr)->spec;
    list->vec = vec_dup((*varr)->vec);

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

/*
 * Do not use this method, but the in-line method ti_varr_eq() instead
 */
_Bool ti__varr_eq(ti_varr_t * varra, ti_varr_t * varrb)
{
    size_t i = 0;

    assert (varra != varrb && varra->vec->n == varrb->vec->n);
    for (vec_each(varra->vec, ti_val_t, va), ++i)
        if (!ti_opr_eq(va, vec_get(varrb->vec, i)))
            return false;
    return true;
}

