#include <ti/fn/fn.h>

#define PROCEDURE_DOC_DOC_ TI_SEE_DOC("#procedure_doc")

static int do__f_procedure_doc(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query->rval == NULL);

    ti_procedure_t * procedure;
    vec_t * procedures = query->target
            ? query->target->procedures
            : ti()->procedures;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `procedure_doc` takes 1 argument but %d were given"
                PROCEDURE_DOC_DOC_, nargs);
        return e->nr;
    }

    if (ti_do_scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `procedure_doc` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead"
                PROCEDURE_DOC_DOC_,
                ti_val_str(query->rval));
        return e->nr;
    }

    procedure = ti_procedures_by_name(procedures, (ti_raw_t *) query->rval);
    if (!procedure)
    {
        ex_set(e, EX_INDEX_ERROR, "procedure `%.*s` not found",
                (int) ((ti_raw_t *) query->rval)->n,
                (char *) ((ti_raw_t *) query->rval)->data);
        return e->nr;
    }

    ti_val_drop(query->rval);

    query->rval = (ti_val_t *) ti_closure_doc(procedure->closure);

    return e->nr;
}
