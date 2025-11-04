#include <ti/fn/fn.h>

int fn_arg_str_slow(
        const char * name,
        const char * doc,
        int argn,
        ti_val_t * val,
        ex_t * e)
{
    if (!ti_val_is_str(val))
        ex_set(e, EX_TYPE_ERROR,
            "function `%s` expects argument %d to be of "
            "type `"TI_VAL_STR_S"` but got type `%s` instead%s",
            name, argn, ti_val_str(val), doc);
    return e->nr;
}

int fn_commit(const char * name, ti_query_t * query, ex_t * e)
{
    if (ti_query_commits(query) && !query->commit)
        ex_set(e, EX_OPERATION,
            "function `%s` requires a commit "
            "before it can be used in the `%s` scope"DOC_COMMIT,
             name,
             ti_query_scope_name(query));
    return e->nr;
}