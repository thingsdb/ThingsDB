#ifndef TI_FN_FN_H_
#define TI_FN_FN_H_

#include <assert.h>
#include <ctype.h>
#include <doc.h>
#include <doc.inline.h>
#include <langdef/langdef.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/access.h>
#include <ti/ano.h>
#include <ti/async.h>
#include <ti/auth.h>
#include <ti/closure.h>
#include <ti/closure.inline.h>
#include <ti/collection.inline.h>
#include <ti/collections.h>
#include <ti/condition.h>
#include <ti/datetime.h>
#include <ti/deep.h>
#include <ti/do.h>
#include <ti/dump.h>
#include <ti/enum.inline.h>
#include <ti/enums.inline.h>
#include <ti/export.h>
#include <ti/field.h>
#include <ti/flags.h>
#include <ti/future.h>
#include <ti/future.inline.h>
#include <ti/gc.h>
#include <ti/item.h>
#include <ti/item.t.h>
#include <ti/member.h>
#include <ti/member.inline.h>
#include <ti/method.h>
#include <ti/commit.h>
#include <ti/commits.h>
#include <ti/mod/expose.h>
#include <ti/mod/expose.t.h>
#include <ti/mod/github.h>
#include <ti/module.h>
#include <ti/module.t.h>
#include <ti/modules.h>
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
#include <ti/room.h>
#include <ti/room.inline.h>
#include <ti/scope.h>
#include <ti/spec.inline.h>
#include <ti/task.h>
#include <ti/tasks.h>
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
#include <ti/vset.inline.h>
#include <ti/vtask.h>
#include <ti/vtask.inline.h>
#include <ti/warn.h>
#include <ti/whitelist.h>
#include <ti/wprop.t.h>
#include <ti/wrap.h>
#include <ti/wrap.inline.h>
#include <tiinc.h>
#include <util/buf.h>
#include <util/cryptx.h>
#include <util/fx.h>
#include <util/mpjson.h>
#include <util/rbuf.h>
#include <util/strx.h>
#include <util/util.h>
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
int fn_arg_int_slow(
        const char * name,
        const char * doc,
        int argn,
        ti_val_t * val,
        ex_t * e);

static inline int fn_get_nargs(cleri_node_t * nd)
{
    return (int) ((intptr_t) nd->data);
}

static inline int fn_is_not_node_scope(ti_query_t * query)
{
    return \
        query->qbind.flags & (TI_QBIND_FLAG_THINGSDB|TI_QBIND_FLAG_COLLECTION);
}

static inline ti_tz_t * fn_default_tz(ti_query_t * query)
{
    return query->collection
            ? query->collection->tz
            : query->qbind.flags & TI_QBIND_FLAG_THINGSDB
            ? ti.t_tz
            : ti.n_tz;
}

static inline int fn_not_node_scope(
        const char * name,
        ti_query_t * query,
        ex_t * e)
{
    if (fn_is_not_node_scope(query))
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

static inline int fn_arg_number(
        const char * name,
        const char * doc,
        int argn,
        ti_val_t * val,
        ex_t * e)
{
    if (!ti_val_is_number(val))
        ex_set(e, EX_TYPE_ERROR,
            "function `%s` expects argument %d to be of "
            "type `"TI_VAL_INT_S"` or `"TI_VAL_FLOAT_S"` "
            "but got type `%s` instead%s",
            name, argn, ti_val_str(val), doc);
    return e->nr;
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

static inline int fn_arg_str_or_ano(
        const char * name,
        const char * doc,
        int argn,
        ti_val_t * val,
        ex_t * e)
{
    if (!ti_val_is_str(val))
        ex_set(e, EX_TYPE_ERROR,
            "function `%s` expects argument %d to be of "
            "type `"TI_VAL_STR_S"` or `"TI_VAL_ANO_S"` "
            "but got type `%s` instead%s",
            name, argn, ti_val_str(val), doc);
    return e->nr;
}


static inline int fn_arg_bytes(
        const char * name,
        const char * doc,
        int argn,
        ti_val_t * val,
        ex_t * e)
{
    if (!ti_val_is_bytes(val))
        ex_set(e, EX_TYPE_ERROR,
            "function `%s` expects argument %d to be of "
            "type `"TI_VAL_BYTES_S"` but got type `%s` instead%s",
            name, argn, ti_val_str(val), doc);
    return e->nr;
}

static inline int fn_arg_str_nil(
        const char * name,
        const char * doc,
        int argn,
        ti_val_t * val,
        ex_t * e)
{
    if (!ti_val_is_str_nil(val))
        ex_set(e, EX_TYPE_ERROR,
            "function `%s` expects argument %d to be of "
            "type `"TI_VAL_STR_S"` or `"TI_VAL_NIL_S"` "
            "but got type `%s` instead%s",
            name, argn, ti_val_str(val), doc);
    return e->nr;
}

static inline int fn_arg_str_bytes_nil(
        const char * name,
        const char * doc,
        int argn,
        ti_val_t * val,
        ex_t * e)
{
    if (!ti_val_is_str_bytes_nil(val))
        ex_set(e, EX_TYPE_ERROR,
            "function `%s` expects argument %d to be of "
            "type `"TI_VAL_STR_S"`, `"TI_VAL_BYTES_S"` or `"TI_VAL_NIL_S"` "
            "but got type `%s` instead%s",
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

static inline int fn_arg_float(
        const char * name,
        const char * doc,
        int argn,
        ti_val_t * val,
        ex_t * e)
{
    if (!ti_val_is_float(val))
        ex_set(e, EX_TYPE_ERROR,
            "function `%s` expects argument %d to be of "
            "type `"TI_VAL_FLOAT_S"` but got type `%s` instead%s",
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

static inline int fn_arg_datetime(
        const char * name,
        const char * doc,
        int argn,
        ti_val_t * val,
        ex_t * e)
{
    if (!ti_val_is_datetime(val))
        ex_set(e, EX_TYPE_ERROR,
            "function `%s` expects argument %d to be of "
            "type `"TI_VAL_DATETIME_S"` or `"TI_VAL_TIMEVAL_S"` "
            "but got type `%s` instead%s",
            name, argn, ti_val_str(val), doc);
    return e->nr;
}

static inline int fn_arg_str_regex(
        const char * name,
        const char * doc,
        int argn,
        ti_val_t * val,
        ex_t * e)
{
    if (!ti_val_is_str_regex(val))
        ex_set(e, EX_TYPE_ERROR,
            "function `%s` expects argument %d to be of "
            "type `"TI_VAL_STR_S"` or `"TI_VAL_REGEX_S"` "
            "but got type `%s` instead%s",
            name, argn, ti_val_str(val), doc);
    return e->nr;
}

static inline int fn_arg_str_closure(
        const char * name,
        const char * doc,
        int argn,
        ti_val_t * val,
        ex_t * e)
{
    if (!ti_val_is_str_closure(val))
        ex_set(e, EX_TYPE_ERROR,
            "function `%s` expects argument %d to be of "
            "type `"TI_VAL_STR_S"` or `"TI_VAL_CLOSURE_S"` "
            "but got type `%s` instead%s",
            name, argn, ti_val_str(val), doc);
    return e->nr;
}

static inline int fn_arg_array(
        const char * name,
        const char * doc,
        int argn,
        ti_val_t * val,
        ex_t * e)
{
    if (!ti_val_is_array(val))
        ex_set(e, EX_TYPE_ERROR,
            "function `%s` expects argument %d to be of "
            "type `"TI_VAL_LIST_S"` or `"TI_VAL_TUPLE_S"` "
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

static inline int fn_commit(const char * name, ti_query_t * query, ex_t * e)
{
    if (*ti_query_commits(query) && !query->commit)
        ex_set(e, EX_OPERATION,
            "function `%s` requires a commit "
            "before it can be used in the `%s` scope"DOC_COMMIT,
             name,
             ti_query_scope_name(query));
    return e->nr;
}

static int fn_call(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    cleri_node_t * child = nd->children;    /* first in argument list */
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

            if (ti_do_statement(query, child, e))
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
    ti_thing_t * thing = (ti_thing_t *) query->rval;
    ti_val_t * val = ti_thing_val_by_strn(thing, name, n);

    if (!val)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "thing "TI_THING_ID" has no property or method `%.*s`",
                thing->id, (int) n, name);
        return e->nr;
    }

    if (!ti_val_is_closure(val))
    {
        ex_set(e, EX_TYPE_ERROR,
            "property `%.*s` is of type `%s` and is not callable",
            (int) n, name, ti_val_str(val));
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
    ti_method_t * method;
    ti_val_t * val;

    if (!name_)
        goto no_prop_err;

    if ((method = ti_type_get_method(thing->via.type, name_)))
        return ti_method_call(method, thing->via.type, query, nd, e);

    if ((val = ti_thing_t_val_weak_get(thing, name_)))
    {
        if (!ti_val_is_closure(val))
        {
            ex_set(e, EX_TYPE_ERROR,
                "property `%.*s` is of type `%s` and is not callable",
                (int) n, name, ti_val_str(val));
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
            thing->via.type->name, (int) n, name);
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
                ti_val_str(query->rval), (int) n, name);
        return e->nr;
    }

    name_ = ti_names_weak_get_strn(name, n);
    if (!name_)
        goto no_method_err;

    method = ti_type_get_method(type, name_);
    if (!method)
        goto no_method_err;

    ti_incref(thing);
    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) thing;

    return ti_method_call(method, type, query, nd, e);

no_method_err:
    ex_set(e, EX_LOOKUP_ERROR,
            "type `%s` has no method `%.*s`",
            type->name, (int) n, name);
    return e->nr;
}

static int fn_call_wa_try_n(
        const char * name,
        size_t n,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    ti_name_t * name_;
    ti_method_t * method;
    ti_wano_t * wano = (ti_wano_t *) query->rval;
    ti_thing_t * thing = wano->thing;

    name_ = ti_names_weak_get_strn(name, n);
    if (!name_)
        goto no_method_err;

    method = ti_type_get_method(wano->ano->type, name_);
    if (!method)
        goto no_method_err;

    ti_incref(thing);
    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) thing;

    return ti_method_call(method, wano->ano->type, query, nd, e);

no_method_err:
    ex_set(e, EX_LOOKUP_ERROR,
            "type `%s` has no method `%.*s`",
            wano->ano->type->name, (int) n, name);
    return e->nr;
}

static int fn_call_a_try_n(
        const char * name,
        size_t n,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    ti_name_t * name_;
    ti_method_t * method;
    ti_ano_t * ano = (ti_ano_t *) query->rval;

    name_ = ti_names_weak_get_strn(name, n);
    if (!name_)
        goto no_method_err;

    method = ti_type_get_method(ano->type, name_);
    if (!method)
        goto no_method_err;


    ti_incref(method->closure);
    ti_val_unsafe_drop(query->rval);
    query->rval = (ti_val_t *) method->closure;
    // TODO: ANO: test
    return fn_call(query, nd, e);

no_method_err:
    ex_set(e, EX_LOOKUP_ERROR,
            "type `%s` has no method `%.*s`",
            ano->type->name, (int) n, name);
    return e->nr;
}

static int fn_call_f_try_n(
        const char * name,
        size_t n,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    ti_module_t * module = (ti_module_t *) query->rval;
    ti_mod_expose_t * expose = ti_mod_expose_by_strn(module, name, n);

    if (expose)
        return expose->closure
                ? ti_mod_closure_call(expose, query, nd, e)
                : ti_mod_expose_call(expose, query, nd, e);

    ex_set(e, EX_LOOKUP_ERROR,
            "module `%s` has no function `%.*s`",
            module->name->str, (int) n, name);
    return e->nr;
}

static int fn_call_m_try_n(
        const char * name,
        size_t n,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    ti_member_t * member = (ti_member_t *) query->rval;
    ti_name_t * needle = ti_names_weak_get_strn(name, n);

    if (needle)
    {
        for (vec_each(member->enum_->methods, ti_method_t, method))
        {
            if (method->name == needle)
            {
                ti_type_t * type = (ti_type_t *) member->enum_;
                return ti_method_call(method, type, query, nd, e);
            }
        }
    }

    ex_set(e, EX_LOOKUP_ERROR,
            "enum `%s` has no function `%.*s`",
            member->enum_->name, (int) n, name);
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

    if (ti_val_is_wano(query->rval))
        return fn_call_wa_try_n(name, n, query, nd, e);

    if (ti_val_is_ano(query->rval))
        return fn_call_a_try_n(name, n, query, nd, e);

    if (ti_val_is_module(query->rval))
        return fn_call_f_try_n(name, n, query, nd, e);

    if (ti_val_is_member(query->rval))
        return fn_call_m_try_n(name, n, query, nd, e);

    ex_set(e, EX_LOOKUP_ERROR,
            "type `%s` has no function `%.*s`",
            ti_val_str(query->rval), (int) n, name);
    return e->nr;
}

#endif /* TI_FN_FN_H_ */
