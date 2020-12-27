#ifndef TI_FN_FN_H_
#define TI_FN_FN_H_

/* maximum value we allow for the `deep` argument */
#define MAX_DEEP_HINT 0x7f

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
#include <ti/datetime.h>
#include <ti/do.h>
#include <ti/enum.inline.h>
#include <ti/enums.inline.h>
#include <ti/export.h>
#include <ti/ext.h>
#include <ti/ext/async.h>
#include <ti/field.h>
#include <ti/future.h>
#include <ti/future.inline.h>
#include <ti/gc.h>
#include <ti/member.h>
#include <ti/member.inline.h>
#include <ti/method.h>
#include <ti/names.h>
#include <ti/nil.h>
#include <ti/opr.h>
#include <ti/procedures.h>
#include <ti/prop.h>
#include <ti/qbind.h>
#include <ti/query.inline.h>
#include <ti/raw.inline.h>
#include <ti/regex.h>
#include <ti/restore.h>
#include <ti/scope.h>
#include <ti/task.h>
#include <ti/thing.inline.h>
#include <ti/token.h>
#include <ti/types.inline.h>
#include <ti/users.h>
#include <ti/val.inline.h>
#include <ti/vbool.h>
#include <ti/verror.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <ti/vset.h>
#include <ti/warn.h>
#include <ti/wprop.t.h>
#include <ti/wrap.h>
#include <ti/wrap.inline.h>
#include <tiinc.h>
#include <util/cryptx.h>
#include <util/strx.h>
#include <util/util.h>
#include <util/fx.h>
#include <uv.h>

typedef int (*fn_cb) (ti_query_t *, cleri_node_t *, ex_t *);
static int fn_call_try_n(
        const char *,
        size_t,
        ti_query_t *,
        cleri_node_t *,
        ex_t *);

#define fn_call_try(__name, __q, __nd, __e) \
    fn_call_try_n(__name, strlen(__name), __q, __nd, __e)

int fn_arg_str_slow(
        const char * name,
        const char * doc,
        int argn,
        ti_val_t * val,
        ex_t * e);

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
            "function `%s` expects argument %d to be of "
            "type `"TI_VAL_STR_S"` but got type `%s` instead%s",
            name, argn, ti_val_str(val), doc);
    return e->nr;
}

static inline int fn_arg_int(
        const char * name,
        const char * doc,
        int argn,
        ti_val_t * val,
        ex_t * e)
{
    if (!ti_val_is_int(val))

        ex_set(e, EX_TYPE_ERROR,
            "function `%s` expects argument %d to be of "
            "type `"TI_VAL_INT_S"` but got type `%s` instead%s",
            name, argn, ti_val_str(val), doc);
    return e->nr;
}

static inline int fn_arg_thing(
        const char * name,
        const char * doc,
        int argn,
        ti_val_t * val,
        ex_t * e)
{
    if (!ti_val_is_thing(val))

        ex_set(e, EX_TYPE_ERROR,
            "function `%s` expects argument %d to be of "
            "type `"TI_VAL_THING_S"` but got type `%s` instead%s",
            name, argn, ti_val_str(val), doc);
    return e->nr;
}


static inline int fn_arg_closure(
        const char * name,
        const char * doc,
        int argn,
        ti_val_t * val,
        ex_t * e)
{
    if (!ti_val_is_closure(val))

        ex_set(e, EX_TYPE_ERROR,
            "function `%s` expects argument %d to be of "
            "type `"TI_VAL_CLOSURE_S"` but got type `%s` instead%s",
            name, argn, ti_val_str(val), doc);
    return e->nr;
}

static inline int fn_arg_bool(
        const char * name,
        const char * doc,
        int argn,
        ti_val_t * val,
        ex_t * e)
{
    if (!ti_val_is_bool(val))

        ex_set(e, EX_TYPE_ERROR,
            "function `%s` expects argument %d to be of "
            "type `"TI_VAL_BOOL_S"` but got type `%s` instead%s",
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

static int fn_call(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    cleri_children_t * child = nd->children;    /* first in argument list */
    ti_closure_t * closure;
    vec_t * args = NULL;

    if (!ti_val_is_closure(query->rval))
        return fn_call_try("call", query, nd, e);

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (closure->vars->n)
    {
        uint32_t n = closure->vars->n;

        args = vec_new(n);
        if (!args)
        {
            ex_set_mem(e);
            goto fail0;
        }

        while (child && n)
        {
            --n;  // outside `while` so we do not go below zero

            if (ti_do_statement(query, child->node, e) ||
                ti_val_make_variable(&query->rval, e))
                goto fail1;

            VEC_push(args, query->rval);
            query->rval = NULL;

            if (!child->next)
                break;

            child = child->next->next;
        }

        while (n--)
            VEC_push(args, ti_nil_get());
    }

    (void) ti_closure_call(closure, query, args, e);

fail1:
    vec_destroy(args, (vec_destroy_cb) ti_val_unsafe_drop);
fail0:
    ti_val_unsafe_drop((ti_val_t *) closure);
    return e->nr;
}

static int fn_call_o_try_n(
        const char * name,
        size_t n,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    ti_name_t * name_ = ti_names_weak_get_strn(name, n);
    ti_thing_t * thing = (ti_thing_t *) query->rval;
    ti_val_t * val;

    if (!name_ || !(val = ti_thing_o_val_weak_get(thing, name_)))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "thing "TI_THING_ID" has no property `%.*s`",
                thing->id, n, name);
        return e->nr;
    }

    if (!ti_val_is_closure(val))
    {
        ex_set(e, EX_TYPE_ERROR,
            "property `%.*s` is of type `%s` and is not callable",
            n, name, ti_val_str(val));
        return e->nr;
    }

    query->rval = val;
    ti_incref(val);
    ti_val_unsafe_drop((ti_val_t *) thing);

    return fn_call(query, nd, e);
}

static int fn_call_t_try_n(
        const char * name,
        size_t n,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    ti_name_t * name_ = ti_names_weak_get_strn(name, n);
    ti_thing_t * thing = (ti_thing_t *) query->rval;
    ti_type_t * type = ti_thing_type(thing);
    ti_method_t * method;
    ti_val_t * val;

    if (!name_)
        goto no_prop_err;

    if ((method = ti_method_by_name(type, name_)))
        return ti_method_call(method, type, query, nd, e);

    if ((val = ti_thing_t_val_weak_get(thing, name_)))
    {
        if (!ti_val_is_closure(val))
        {
            ex_set(e, EX_TYPE_ERROR,
                "property `%.*s` is of type `%s` and is not callable",
                n, name, ti_val_str(val));
            return e->nr;
        }

        query->rval = val;
        ti_incref(val);
        ti_val_unsafe_drop((ti_val_t *) thing);

        return fn_call(query, nd, e);
    }

no_prop_err:
    ex_set(e, EX_LOOKUP_ERROR,
            "type `%s` has no property or method `%.*s`",
            type->name, n, name);
    return e->nr;
}

static int fn_call_w_try_n(
        const char * name,
        size_t n,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    ti_name_t * name_;
    ti_method_t * method;
    ti_wrap_t * wrap = (ti_wrap_t *) query->rval;
    ti_thing_t * thing = wrap->thing;
    ti_type_t * type = ti_wrap_maybe_type(wrap);

    if (!type)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "type `%s` has no function `%.*s`",
                ti_val_str(query->rval), n, name);
        return e->nr;
    }

    name_ = ti_names_weak_get_strn(name, n);
    if (!name_)
        goto no_method_err;

    method = ti_method_by_name(type, name_);
    if (!method)
        goto no_method_err;

    ti_incref(thing);
    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) thing;

    return ti_method_call(method, type, query, nd, e);

no_method_err:
    ex_set(e, EX_LOOKUP_ERROR,
            "type `%s` has no method `%.*s`",
            type->name, n, name);
    return e->nr;
}

#define fn_call_try(__name, __q, __nd, __e) \
    fn_call_try_n(__name, strlen(__name), __q, __nd, __e)

static int fn_call_try_n(
        const char * name,
        size_t n,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    if (ti_val_is_thing(query->rval))
        return ti_thing_is_object((ti_thing_t *) query->rval)
                ? fn_call_o_try_n(name, n, query, nd, e)
                : fn_call_t_try_n(name, n, query, nd, e);

    if (ti_val_is_wrap(query->rval))
        return fn_call_w_try_n(name, n, query, nd, e);

    ex_set(e, EX_LOOKUP_ERROR,
            "type `%s` has no function `%.*s`",
            ti_val_str(query->rval), n, name);
    return e->nr;
}

#endif /* TI_FN_FN_H_ */
