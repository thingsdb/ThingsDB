#ifndef TI_FN_FN_H_
#define TI_FN_FN_H_

#include <assert.h>
#include <ctype.h>
#include <doc.h>
#include <doc.inline.h>
#include <langdef/langdef.h>
#include <langdef/nd.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/access.h>
#include <ti/auth.h>
#include <ti/closure.h>
#include <ti/closure.inline.h>
#include <ti/collections.h>
#include <ti/do.h>
#include <ti/field.h>
#include <ti/names.h>
#include <ti/nil.h>
#include <ti/opr.h>
#include <ti/procedures.h>
#include <ti/prop.h>
#include <ti/qbind.h>
#include <ti/query.inline.h>
#include <ti/raw.inline.h>
#include <ti/regex.h>
#include <ti/scope.h>
#include <ti/task.h>
#include <ti/thing.inline.h>
#include <ti/types.inline.h>
#include <ti/users.h>
#include <ti/val.inline.h>
#include <ti/vbool.h>
#include <ti/verror.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <ti/vset.h>
#include <ti/warn.h>
#include <ti/wprop.h>
#include <ti/wrap.h>
#include <tiinc.h>
#include <util/cryptx.h>
#include <util/strx.h>
#include <util/util.h>
#include <uv.h>


static inline int fn_not_node_scope(
        const char * name,
        ti_query_t * query,
        ex_t * e)
{
    if (~query->qbind.flags & TI_QBIND_FLAG_NODE)
        ex_set(e, EX_LOOKUP_ERROR,
            "function `%s` is undefined in the `%s` scope; "
            "you might want to query a `@node` scope?"DOC_SCOPES,
            name,
            ti_query_scope_name(query));
    return e->nr;
}

static inline int fn_not_thingsdb_scope(
        const char * name,
        ti_query_t * query,
        ex_t * e)
{
    if (~query->qbind.flags & TI_QBIND_FLAG_THINGSDB)
        ex_set(e, EX_LOOKUP_ERROR,
            "function `%s` is undefined in the `%s` scope; "
            "you might want to query the `@thingsdb` scope?"DOC_SCOPES,
            name,
            ti_query_scope_name(query));
    return e->nr;
}

static inline int fn_nargs(
        const char * name,
        const char * doc,
        const int n,
        const int nargs,
        ex_t * e)
{
    if (nargs != n)
        ex_set(e, EX_NUM_ARGUMENTS,
            "function `%s` takes %d argument%s but %d %s given%s",
            name, n, n==1 ? "" : "s", nargs, nargs==1 ? "was" : "were", doc);
    return e->nr;
}

static inline int fn_nargs_max(
        const char * name,
        const char * doc,
        const int ma,
        const int nargs,
        ex_t * e)
{
    if (nargs > ma)
        ex_set(e, EX_NUM_ARGUMENTS,
            "function `%s` takes at most %d argument%s but %d %s given%s",
            name, ma, ma==1 ? "" : "s", nargs, nargs==1 ? "was" : "were", doc);
    return e->nr;
}

static inline int fn_nargs_min(
        const char * name,
        const char * doc,
        const int mi,
        const int nargs,
        ex_t * e)
{
    if (nargs < mi)
        ex_set(e, EX_NUM_ARGUMENTS,
            "function `%s` requires at least %d argument%s but %d %s given%s",
            name, mi, mi==1 ? "" : "s", nargs, nargs==1 ? "was" : "were", doc);
    return e->nr;
}

static inline int fn_nargs_range(
        const char * name,
        const char * doc,
        const int mi,
        const int ma,
        const int nargs,
        ex_t * e)
{
    return (
            fn_nargs_min(name, doc, mi, nargs, e) ||
            fn_nargs_max(name, doc, ma, nargs, e)
    ) ? e->nr : 0;
}

static inline int fn_arg_str(
        const char * name,
        const char * doc,
        int argn,
        ti_val_t * val,
        ex_t * e)
{
    if (!ti_val_is_str(val))

        ex_set(e, EX_TYPE_ERROR,
            "function `%s` expects argument %d to be of type `"TI_VAL_STR_S"` "
            "but got type `%s` instead%s",
            name, argn, ti_val_str(val), doc);
    return e->nr;
}

static inline int fn_arg_name_check(
        const char * name,
        const char * doc,
        int argn,
        ti_val_t * val,
        ex_t * e)
{
    if (fn_arg_str(name, doc, argn, val, e))
        return e->nr;
    if (!ti_name_is_valid_strn(
            (const char *) ((ti_raw_t *) val)->data,
            ((ti_raw_t *) val)->n))
        ex_set(e, EX_VALUE_ERROR,
            "function `%s` expects argument %d to follow the naming rules"
            DOC_NAMES, name, argn);
    return e->nr;
}

static inline int fn_not_collection_scope(
        const char * name,
        ti_query_t * query,
        ex_t * e)
{
    if (~query->qbind.flags & TI_QBIND_FLAG_COLLECTION)
        ex_set(e, EX_LOOKUP_ERROR,
            "function `%s` is undefined in the `%s` scope; "
            "you might want to query a `@collection` scope?"DOC_SCOPES,
            name,
            ti_query_scope_name(query));
    return e->nr;
}

static inline int fn_not_thingsdb_or_collection_scope(
        const char * name,
        ti_query_t * query,
        ex_t * e)
{
    if (!(query->qbind.flags &
            (TI_QBIND_FLAG_THINGSDB|TI_QBIND_FLAG_COLLECTION)))
        ex_set(e, EX_LOOKUP_ERROR,
            "function `%s` is undefined in the `%s` scope; "
            "you might want to query the `@thingsdb` "
            "or a `@collection` scope?"DOC_SCOPES,
            name,
            ti_query_scope_name(query));

    return e->nr;
}

#endif /* TI_FN_FN_H_ */
