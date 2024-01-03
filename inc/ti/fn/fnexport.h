#include <ti/fn/fn.h>


typedef struct
{
    _Bool * dump;
    ex_t * e;
} export__walk_t;


static int do__export_option(
        ti_raw_t * key,
        ti_val_t * val,
        export__walk_t * w)
{
    if (ti_raw_eq_strn(key, "dump", 4))
    {
        if (!ti_val_is_bool(val))
        {
            ex_set(w->e, EX_TYPE_ERROR,
                    "dump must be of type `"TI_VAL_BOOL_S"` but "
                    "got type `%s` instead"DOC_EXPORT,
                    ti_val_str(val));
            return w->e->nr;
        }
        *w->dump = ti_val_as_bool(val);
        return 0;
    }

    ex_set(w->e, EX_VALUE_ERROR,
            "invalid export option `%.*s`"DOC_EXPORT, key->n, key->data);

    return w->e->nr;
}

static int do__f_export(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    _Bool dump = false;

    if (fn_not_collection_scope("export", query, e) ||
        fn_nargs_max("export", DOC_EXPORT, 1, nargs, e))
        return e->nr;

    if (query->change || query->futures.n)
    {
        ex_set(e, EX_OPERATION,
                "function `export` must not be used with a change or future");
        return e->nr;
    }

    if (nargs == 1)
    {
        ti_thing_t * thing;

        if (ti_do_statement(query, nd->children, e) ||
            fn_arg_thing("export", DOC_EXPORT, 1, query->rval, e))
            return e->nr;

        thing = (ti_thing_t *) query->rval;

        export__walk_t w = {
                .dump = &dump,
                .e = e,
        };

        if (ti_thing_walk(thing, (ti_thing_item_cb) do__export_option, &w))
            return e->nr;

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }

    query->rval = dump
            ? (ti_val_t *) ti_dump_collection(query->collection)
            : (ti_val_t *) ti_export_collection(query->collection);

    if (!query->rval)
        ex_set_mem(e);

    return e->nr;
}
