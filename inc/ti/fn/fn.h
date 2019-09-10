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
#include <ti/closure.h>
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
        ex_set(e, EX_INDEX_ERROR, "function `%s` is undefined", name);
    return e->nr;
}

static inline int fn_chained(
        const char * name,
        ti_query_t * query,
        ex_t * e)
{
    if (query->rval)
        ex_set(e, EX_INDEX_ERROR, "type `%s` has no function `%s`",
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
        ex_set(e, EX_INDEX_ERROR,
            "function `%s` is undefined in the `%s` scope; "
            "You might want to query the `node` scope?",
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
        ex_set(e, EX_INDEX_ERROR,
            "function `%s` is undefined in the `%s` scope; "
            "You might want to query the `thingsdb` scope?",
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

    if (query->syntax.flags & TI_SYNTAX_FLAG_NODE)
        ex_set(e, EX_INDEX_ERROR,
            "function `%s` is undefined in the `%s` scope; "
            "You might want to query the `thingsdb` or a `collection` scope?",
            name,
            ti_query_scope_name(query));

    return e->nr;
}


#endif /* TI_FN_FN_H_ */
