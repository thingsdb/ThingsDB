#include <ti/fn/fn.h>

static int do__f_dict(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_dict_t * dict;

    if (fn_nargs_max("dict", DOC_DICT, 1, nargs, e))
        return e->nr;

    dict = ti_dict_create();
    if (!dict)
    {
        ex_set_mem(e);
        return e->nr;
    }

    if (nargs == 1)
    {
        if (ti_do_statement(query, nd->children, e) ||
            fn_arg_array("dict", DOC_DICT, 1, query->rval, e))
            goto fail0;

        for (vec_each(VARR(query->rval), ti_val_t, tuple))
        {
            if (!ti_val_is_array(tuple) || VARR(tuple)->n != 2)
            {
                ex_set(e, EX_VALUE_ERROR,
                    "type `dict` must be initialized with a "
                    "list of [key, value] pairs."DOC_DICT);
                goto fail0;
            }
            ti_key_t * key = VEC_get(VARR(tuple), 0);
            ti_key_t * val = VEC_get(VARR(tuple), 1);
            if (ti_dict_set(dict, key, val, e))
                goto fail0;
        }
    }

    query->rval = (ti_val_t *) dict;
    return e->nr;
fail0:
    ti_dict_destroy(dict);
    return e->nr;
}
