#include <ti/fn/fn.h>

#define COLLECTION_DOC_ TI_SEE_DOC("#collection")

static int do__f_collection(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (query->syntax.flags & TI_SYNTAX_FLAG_THINGSDB);
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    ti_collection_t * collection;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `collection` takes 1 argument but %d were given"
                COLLECTION_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_scope(query, nd->children->node, e))
        return e->nr;

    collection = ti_collections_get_by_val(query->rval, e);
    if (e->nr)
        return e->nr;

    assert (collection);


    ti_val_drop(query->rval);
    query->rval = ti_collection_as_qpval(collection);
    if (!query->rval)
        ex_set_alloc(e);

    return e->nr;
}
