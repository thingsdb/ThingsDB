#include <ti/fn/fn.h>

static int do__f_rename_procedure(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_task_t * task;
    ti_procedure_t * procedure;
    ti_raw_t * nname;
    smap_t * procedures = ti_query_procedures(query);

    if (fn_not_thingsdb_or_collection_scope("rename_procedure", query, e) ||
        fn_nargs("rename_procedure", DOC_RENAME_PROCEDURE, 2, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_str("rename_procedure", DOC_RENAME_PROCEDURE, 1, query->rval, e))
        return e->nr;

    procedure = ti_procedures_by_name(procedures, (ti_raw_t *) query->rval);
    if (!procedure)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "procedure", e);

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->next->next, e) ||
        fn_arg_str_slow(
                "rename_procedure",
                DOC_RENAME_PROCEDURE,
                2,
                query->rval,
                e))
        return e->nr;

    nname = (ti_raw_t *) query->rval;
    query->rval = (ti_val_t *) ti_nil_get();

    if (!ti_name_is_valid_strn((const char *) nname->data, nname->n))
    {
        ex_set(e, EX_VALUE_ERROR,
                "procedure name must follow the naming rules"DOC_NAMES);
        goto fail0;
    }

    if (ti_procedures_by_name(procedures, nname))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "procedure `%.*s` already exists",
                nname->n, (const char *) nname->data);
        goto fail0;
    }

    task = ti_task_get_task(
            query->change,
            query->collection ? query->collection->root : ti.thing0);

    if (!task ||
        ti_task_add_rename_procedure(task, procedure, nname) ||
        ti_procedures_rename(
                        procedures,
                        procedure,
                        (const char *) nname->data,
                        nname->n))
        ex_set_mem(e);  /* task cleanup is not required */

fail0:
    ti_val_unsafe_drop((ti_val_t *) nname);
    return e->nr;
}
