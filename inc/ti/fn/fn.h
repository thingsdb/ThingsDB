#ifndef TI_FN_FN_H_
#define TI_FN_FN_H_

#include <assert.h>
#include <langdef/langdef.h>
#include <langdef/nd.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/access.h>
#include <ti/auth.h>
#include <ti/vali.h>
#include <ti/closure.h>
#include <ti/closurei.h>
#include <ti/collections.h>
#include <ti/do.h>
#include <ti/names.h>
#include <ti/nil.h>
#include <ti/opr.h>
#include <ti/procedures.h>
#include <ti/prop.h>
#include <ti/regex.h>
#include <ti/syntax.h>
#include <ti/task.h>
#include <ti/users.h>
#include <ti/vbool.h>
#include <ti/vfloat.h>
#include <ti/verror.h>
#include <ti/vint.h>
#include <ti/vset.h>
#include <ti/warn.h>
#include <ti/scope.h>
#include <ti/field.h>
#include <ti/thingi.h>
#include <tiinc.h>
#include <util/cryptx.h>
#include <util/strx.h>
#include <util/util.h>
#include <uv.h>


static inline int fn_not_chained(
        const char * name,
        ti_query_t * query,
        ex_t * e)
{
    if (!query->rval)
        ex_set(e, EX_LOOKUP_ERROR, "function `%s` is undefined", name);
    return e->nr;
}

static inline int fn_chained(
        const char * name,
        ti_query_t * query,
        ex_t * e)
{
    if (query->rval)
        ex_set(e, EX_LOOKUP_ERROR, "type `%s` has no function `%s`",
            ti_val_str(query->rval),
            name);
    return e->nr;
}

static inline int fn_not_node_scope(
        const char * name,
        ti_query_t * query,
        ex_t * e)
{
    if (fn_chained(name, query, e))
        return e->nr;

    if (~query->syntax.flags & TI_SYNTAX_FLAG_NODE)
        ex_set(e, EX_LOOKUP_ERROR,
            "function `%s` is undefined in the `%s` scope; "
            "you might want to query a `@node` scope?"SCOPES_DOC_,
            name,
            ti_query_scope_name(query));
    return e->nr;
}

static inline int fn_not_thingsdb_scope(
        const char * name,
        ti_query_t * query,
        ex_t * e)
{
    if (fn_chained(name, query, e))
        return e->nr;

    if (~query->syntax.flags & TI_SYNTAX_FLAG_THINGSDB)
        ex_set(e, EX_LOOKUP_ERROR,
            "function `%s` is undefined in the `%s` scope; "
            "you might want to query the `@thingsdb` scope?"SCOPES_DOC_,
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
        const int mi,
        const int ma,
        const int nargs,
        ex_t * e)
{
    if (nargs < mi)
        ex_set(e, EX_NUM_ARGUMENTS,
            "function `%s` requires at least %d argument%s but %d %s given%s",
            name, mi, mi==1 ? "" : "s", nargs, nargs==1 ? "was" : "were", doc);
    else if (nargs > ma)
        ex_set(e, EX_NUM_ARGUMENTS,
            "function `%s` takes at most %d argument%s but %d %s given%s",
            name, ma, ma==1 ? "" : "s", nargs, nargs==1 ? "was" : "were", doc);
    return e->nr;
}

static inline int fn_not_collection_scope(
        const char * name,
        ti_query_t * query,
        ex_t * e)
{
    if (fn_chained(name, query, e))
        return e->nr;

    if (~query->syntax.flags & TI_SYNTAX_FLAG_COLLECTION)
        ex_set(e, EX_LOOKUP_ERROR,
            "function `%s` is undefined in the `%s` scope; "
            "you might want to query a `@collection` scope?"SCOPES_DOC_,
            name,
            ti_query_scope_name(query));
    return e->nr;
}

static inline int fn_not_thingsdb_or_collection_scope(
        const char * name,
        ti_query_t * query,
        ex_t * e)
{
    if (fn_chained(name, query, e))
        return e->nr;

    if (!(query->syntax.flags &
            (TI_SYNTAX_FLAG_THINGSDB|TI_SYNTAX_FLAG_COLLECTION)))
        ex_set(e, EX_LOOKUP_ERROR,
            "function `%s` is undefined in the `%s` scope; "
            "you might want to query the `@thingsdb` "
            "or a `@collection` scope?"SCOPES_DOC_,
            name,
            ti_query_scope_name(query));

    return e->nr;
}


#endif /* TI_FN_FN_H_ */
