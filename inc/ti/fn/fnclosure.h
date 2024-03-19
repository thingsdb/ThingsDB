#include <ti/fn/fn.h>

static int do__f_closure_task(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_vtask_t * vtask;

    if (!ti_val_is_task(query->rval))
        return fn_call_try("closure", query, nd, e);

    vtask = (ti_vtask_t *) query->rval;

    if (ti_vtask_is_nil(vtask, e) ||
        fn_nargs("closure", DOC_TASK_CLOSURE, 0, nargs, e))
        return e->nr;

    query->rval = (ti_val_t *) vtask->closure;
    ti_incref(query->rval);

    ti_vtask_unsafe_drop(vtask);
    return e->nr;
}

static int do__f_closure_new(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_str_t * str;
    ti_qbind_t syntax = {
            .immutable_n = 0,
            .flags = query->collection
                ? TI_QBIND_FLAG_COLLECTION
                : TI_QBIND_FLAG_THINGSDB,
    };

    if (fn_nargs_max("closure", DOC_CLOSURE, 1, nargs, e))
        return e->nr;

    if (nargs == 0)
    {
        query->rval = (ti_val_t *) ti_val_default_closure();
        return e->nr;
    }

    if (ti_do_statement(query, nd->children, e))
        return e->nr;

    switch((ti_val_enum) query->rval->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_DATETIME:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ROOM:
    case TI_VAL_TASK:
    case TI_VAL_ARR:
    case TI_VAL_SET:
    case TI_VAL_ERROR:
    case TI_VAL_MEMBER:
    case TI_VAL_MPDATA:
    case TI_VAL_FUTURE:
    case TI_VAL_MODULE:
        ex_set(e, EX_TYPE_ERROR,
                "cannot convert type `%s` to `"TI_VAL_CLOSURE_S"`",
                ti_val_str(query->rval));
        break;
    case TI_VAL_CLOSURE:
        break;
    case TI_VAL_STR:
    case TI_VAL_NAME:
        str = (ti_str_t *) query->rval;
        query->rval = (ti_val_t *) ti_closure_from_strn(
                &syntax,
                str->str,
                str->n,
                e);
        ti_val_unsafe_drop((ti_val_t *) str);
        break;
    case TI_VAL_TEMPLATE:
        assert(0);
    }

    return e->nr;
}

static inline int do__f_closure(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    return query->rval
            ? do__f_closure_task(query, nd, e)
            : do__f_closure_new(query, nd, e);
}
