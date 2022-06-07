#include <ti/fn/fn.h>

static int do__f_join(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_varr_t * arr;
    ti_raw_t * sep = NULL;
    size_t n = sizeof(ti_raw_t), idx = 0;
    void * buffer;

    if (!ti_val_is_array(query->rval))
        return fn_call_try("join", query, nd, e);

    if (fn_nargs_max("join", DOC_LIST_JOIN, 1, nargs, e))
        return e->nr;

    arr = (ti_varr_t *) query->rval;
    query->rval = NULL;

    if (nargs == 1 && arr->vec->n > 1)
    {
        /* Only read the separator when the array contains at least 2 items */
        if (ti_do_statement(query, nd->children, e) ||
            fn_arg_str("join", DOC_LIST_JOIN, 1, query->rval, e))
            goto fail0;

        sep = (ti_raw_t *) query->rval;
        query->rval = NULL;

        /*
         * This is safe because this part of the code is only reached when
         * at least two items are in the array.
         */
        n += sep->n * (arr->vec->n-1);
    }

    for (vec_each(arr->vec, ti_val_t, val), ++idx)
    {
        if (!ti_val_is_str(val))
        {
            ex_set(e, EX_TYPE_ERROR,
                    "expecting item %zu to be of type `"TI_VAL_STR_S"` "
                    "but got type `%s` instead"DOC_LIST_JOIN,
                    idx, ti_val_str(val));
            goto fail1;
        }
        n += ((ti_raw_t *) val)->n;
    }

    buffer = malloc(n);
    if (!buffer)
    {
        ex_set_mem(e);
        goto fail1;
    }

    /*
     * Set the return value to the buffer.
     * This allows the code to change the buffer and actually fill the buffer
     * with the correct data.
     */
    ti_raw_init(buffer, TI_VAL_STR, n);
    query->rval = buffer;

    buffer += sizeof(ti_raw_t);

    for (vec_each(arr->vec, ti_raw_t, raw))
    {
        memcpy(buffer, raw->data, raw->n);
        buffer += raw->n;
        if (sep && --idx)
        {
            memcpy(buffer, sep->data, sep->n);
            buffer += sep->n;
        }
    }

fail1:
    ti_val_drop((ti_val_t *) sep);
fail0:
    ti_val_unsafe_drop((ti_val_t *) arr);
    return e->nr;
}
