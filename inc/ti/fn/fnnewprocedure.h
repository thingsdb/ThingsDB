#include <ti/fn/fn.h>

#define NEW_PROCEDURE_DOC_ TI_SEE_DOC("#new_procedure")

static int do__f_new_procedure(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (~query->syntax.flags & TI_SYNTAX_FLAG_NODE);
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    ti_raw_t * raw;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `new_procedure` takes 1 argument but %d were given"
                NEW_PROCEDURE_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `new_procedure` expects argument 1 to be of "
            "type `"TI_VAL_RAW_S"` but got type `%s` instead"
            NEW_PROCEDURE_DOC_,
            ti_val_str(query->rval));
        return e->nr;
    }

    raw = (ti_raw_t *) query->rval;

    if (!strx_is_utf8n((const char *) raw->data, raw->n))
    {
        ex_set(e, EX_BAD_DATA,
                "function `new_procedure` expects a procedure "
                "to have valid UTF8 encoding"NEW_PROCEDURE_DOC_);
        return e->nr;
    }


    return e->nr;
}
