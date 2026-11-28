#include <ti/fn/fn.h>

static int do__f_template(ti_query_t *query, cleri_node_t *nd, ex_t *e)
{
    const int nargs = fn_get_nargs(nd);
    ti_str_t *str;
    ti_qbind_t syntax = {
            .immutable_n = 0,
            .flags = query->collection
                ? TI_QBIND_FLAG_COLLECTION
                : TI_QBIND_FLAG_THINGSDB,
    };

    if (fn_nargs("template", DOC_TEMPLATE, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_str_slow("template", DOC_TEMPLATE, 1, query->rval, e))
        return e->nr;

    str = (ti_str_t *) query->rval;
    query->rval = (ti_val_t *) ti_template_create(&syntax, str->str, str->n, e);
    ti_val_unsafe_drop((ti_val_t *) str);
    return e->nr;
}
