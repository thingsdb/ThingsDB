/*
 * ti/cq.c
 */
#include <assert.h>
#include <langdef/langdef.h>
#include <langdef/nd.h>
#include <math.h>
#include <stdlib.h>
#include <ti/closure.h>
#include <tiinc.h>
#include <ti/cq.h>
#include <ti/names.h>
#include <ti/nil.h>
#include <ti/opr.h>
#include <ti/task.h>
#include <ti/prop.h>
#include <ti/regex.h>
#include <ti/vbool.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <util/strx.h>
#include <util/util.h>

/*
 * Functions:
 *  1. Test if the function is available in scope
 *  2. Test the number of arguments
 *  3. Lazy evaluate each argument
 */
static int cq__array(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__assignment(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__chain(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__chain_name(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__closure(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_assert(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_blob(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_del(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_endswith(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_filter(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_find(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_findindex(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_hasprop(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_id(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_indexof(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_int(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_isarray(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_isascii(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_isbool(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_isfloat(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_isinf(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_isint(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_islist(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_isnan(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_israw(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_istuple(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_isutf8(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_len(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_lower(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_map(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_now(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_pop(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_push(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_refs(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_remove(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_rename(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_ret(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_splice(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_startswith(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_str(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_test(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_t(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_try(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_upper(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__function(
        ti_query_t * query,
        cleri_node_t * nd,
        _Bool is_scope,             /* scope or chain */
        ex_t * e);
static int cq__index(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__operations(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__optscope(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__primitives(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__scope_name(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__scope_thing(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__tmp(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__tmp_assign(ti_query_t * query, cleri_node_t * nd, ex_t * e);


int ti_cq_scope(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_SCOPE);
    assert (query->rval == NULL);

    int nots = 0;
    cleri_node_t * node;
    ti_scope_t * current_scope;
    cleri_children_t * nchild, * child = nd         /* sequence */
            ->children;                             /* first child, not */

    for (nchild = child->node->children; nchild; nchild = nchild->next)
        ++nots;

    child = child->next;
    node = child->node                      /* choice */
            ->children->node;               /* primitives, function,
                                               assignment, name, thing,
                                               array, compare, closure */
    current_scope = query->scope;
    query->scope = ti_scope_enter(current_scope, query->target->root);
    if (!query->scope)
    {
        ex_set_alloc(e);
        return e->nr;
    }

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_ARRAY:
        if (cq__array(query, node, e))
            goto onerror;
        break;
    case CLERI_GID_CLOSURE:
        if (cq__closure(query, node, e))
            goto onerror;
        break;
    case CLERI_GID_ASSIGNMENT:
        if (cq__assignment(query, node, e))
            goto onerror;
        break;
    case CLERI_GID_FUNCTION:
        if (cq__function(query, node, true, e))
            goto onerror;
        break;
    case CLERI_GID_NAME:
        if (cq__scope_name(query, node, e))
            goto onerror;
        break;
    case CLERI_GID_OPERATIONS:
        /* skip the sequence , jump to the priority list */
        if (cq__operations(query, node->children->next->node, e))
            goto onerror;

        if (node->children->next->next->next)   /* optional (sequence) */
        {
            _Bool bool_ = ti_val_as_bool(query->rval);
            ti_val_drop(query->rval);
            query->rval = NULL;
            node = node->children->next->next->next->node;  /* sequence */
            if (ti_cq_scope(
                    query,
                    bool_
                        ? node->children->next->node        /* scope, true */
                        : node->children->next->next->next->node, /* false */
                    e))
                goto onerror;
        }
        break;
    case CLERI_GID_PRIMITIVES:
        if (cq__primitives(query, node, e))
            goto onerror;
        break;
    case CLERI_GID_THING:
        if (cq__scope_thing(query, node, e))
            goto onerror;
        break;
    case CLERI_GID_TMP:
        if (cq__tmp(query, node, e))
            goto onerror;
        break;
    case CLERI_GID_TMP_ASSIGN:
        if (cq__tmp_assign(query, node, e))
            goto onerror;
        break;

    default:
        assert (0);  /* all possible should be handled */
        return -1;
    }

    child = child->next;
    node = child->node;

    /* handle index */
    if (node->children && cq__index(query, node, e))
        goto onerror;

    if (child->next && cq__chain(query, child->next->node, e))
        goto onerror;

    if (!query->rval)
    {
        query->rval = ti_scope_weak_get_val(query->scope);
        ti_incref(query->rval);
    }

    if (nots)
    {
        _Bool b = ti_val_as_bool(query->rval);
        ti_val_drop(query->rval);
        query->rval = (ti_val_t *) ti_vbool_get((nots & 1) ^ b);
    }

onerror:
    ti_scope_leave(&query->scope, current_scope);
    return e->nr;
}

static int cq__array(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_ARRAY);

    ti_varr_t * varr;
    uintptr_t sz = (uintptr_t) nd->data;
    cleri_children_t * child = nd          /* sequence */
            ->children->next->node         /* list */
            ->children;

    if (sz >= query->target->quota->max_array_size)
    {
        ex_set(e, EX_MAX_QUOTA,
                "maximum array size quota of %zu has been reached, "
                "see "TI_DOCS"#quotas", query->target->quota->max_array_size);
        return e->nr;
    }

    varr = ti_varr_create(sz);
    if (!varr)
    {
        ex_set_alloc(e);
        return e->nr;
    }

    for (; child; child = child->next->next)
    {
        if (ti_cq_scope(query, child->node, e))
            goto failed;

        if (ti_varr_append(varr, (void **) &query->rval, e))
            goto failed;

        query->rval = NULL;

        if (!child->next)
            break;
    }

    query->rval = (ti_val_t *) varr;
    return 0;

failed:
    ti_val_drop((ti_val_t *) varr);
    return e->nr;
}

static int cq__assignment(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_ASSIGNMENT);
    assert (query->ev);

    ti_thing_t * thing;
    ti_name_t * name;
    ti_task_t * task = NULL;
    ti_val_t * left_val = NULL;     /* assign to prevent warning */
    size_t max_props = query->target->quota->max_props;
    cleri_node_t * name_nd = nd                 /* sequence */
            ->children->node;                   /* name */
    cleri_node_t * assign_nd = nd               /* sequence */
            ->children->next->node;             /* assign tokens */
    cleri_node_t * scope_nd = nd                /* sequence */
            ->children->next->next->node;       /* scope */

    thing = (ti_thing_t *) ti_query_val_pop(query);

    if (thing->tp != TI_VAL_THING)
    {
        ex_set(e, EX_BAD_DATA, "cannot assign properties to `%s` type",
                ti_val_str((ti_val_t *) thing));
        goto fail0;
    }

    name = ti_names_get(name_nd->str, name_nd->len);
    if (!name)
        goto alloc_err;

    if (assign_nd->len == 1)
    {
        /* assign_nd == '=', a new assignment */
        if (thing->props->n == max_props)
        {
            ex_set(e, EX_MAX_QUOTA,
                    "maximum properties quota of %zu has been reached, "
                    "see "TI_DOCS"#quotas", max_props);
            goto fail1;
        }
    }
    else
    {
        /* apply to existing variable */
        assert (assign_nd->len == 2);
        assert (assign_nd->str[1] == '=');

        left_val = ti_thing_prop_weak_get(thing, name);
        if (!left_val)
        {
            ex_set(e, EX_INDEX_ERROR,
                    "`%.*s` is undefined", (int) name->n, name->str);
            goto fail1;
        }
    }

    /* should also work in chain because then scope->local must be NULL */
    if (    ti_scope_has_local_name(query->scope, name) ||
            ti_scope_in_use_name(query->scope, thing, name))
    {
        ex_set(e, EX_BAD_DATA,
            "cannot assign a new value to `%.*s` while the property is in use",
            (int) name->n, name->str);
        goto fail1;
    }

    if (ti_cq_scope(query, scope_nd, e))
        goto fail1;

    if (assign_nd->len == 2)
    {
        assert (left_val);
        if (ti_opr_a_to_b(left_val, assign_nd, &query->rval, e))
            goto fail1;
    }
    else if (ti_val_make_assignable(&query->rval, e))
        goto fail1;

    if (thing->id)
    {
        task = ti_task_get_task(query->ev, thing, e);

        if (!task)
            goto fail1;

        if (ti_task_add_assign(task, name, query->rval))
            goto alloc_err;  /* we do not need to cleanup task, since the task
                                is added to `query->ev->tasks` */
    }

    if (ti_thing_prop_set(thing, name, query->rval))
    {
        if (task)
            free(vec_pop(task->jobs));
        goto alloc_err;
    }

    ti_incref(query->rval);

    return 0;

alloc_err:
    ex_set_alloc(e);

fail1:
    ti_name_drop(name);

fail0:
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}

static int cq__chain(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_CHAIN);

    cleri_children_t * child = nd           /* sequence */
                    ->children->next;       /* first is .(dot), next choice */

    cleri_node_t * node = child->node       /* choice */
            ->children->node;               /* function, assignment,
                                               name */

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_FUNCTION:
        if (cq__function(query, node, false, e))
            return e->nr;
        break;
    case CLERI_GID_ASSIGNMENT:
        if (cq__assignment(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_NAME:
        if (cq__chain_name(query, node, e))
            return e->nr;
        break;
    default:
        assert (0);  /* all possible should be handled */
        return -1;
    }

    child = child->next;
    node = child->node;

    if (node->children && cq__index(query, node, e))
        return e->nr;

    child = child->next;
    if (!child)
        return e->nr;

    (void) cq__chain(query, child->node, e);
    return e->nr;
}

static int cq__chain_name(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_NAME);
    assert (ti_name_is_valid_strn(nd->str, nd->len));

    ti_name_t * name;
    ti_val_t * val;
    ti_thing_t * thing = (ti_thing_t *) ti_query_val_pop(query);

    if (thing->tp != TI_VAL_THING)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no property `%.*s`",
                ti_val_str((ti_val_t *) thing),
                (int) nd->len, nd->str);
        return e->nr;
    }

    name = ti_names_weak_get(nd->str, nd->len);
    val = name ? ti_thing_prop_weak_get(thing, name) : NULL;

    if (!val)
    {
        ex_set(e, EX_INDEX_ERROR,
                "property `%.*s` on thing "TI_THING_ID" is undefined",
                (int) nd->len, nd->str,
                thing->id);
        return e->nr;
    }

    if (ti_scope_push_name(&query->scope, name, val))
        ex_set_alloc(e);

    return e->nr;
}

static int cq__closure(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_CLOSURE);

    query->rval = (ti_val_t *) ti_closure_from_node(nd);
    if (!query->rval)
        ex_set_alloc(e);

    return e->nr;
}

static int cq__f_assert(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    cleri_children_t * child = nd->children;    /* first in argument list */
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * msg;
    ti_vint_t * code;

    if (nargs < 1)
    {
        ex_set(e, EX_BAD_DATA,
                "function `assert` requires at least 1 argument but 0 "
                "were given");
        return e->nr;
    }

    if (nargs > 3)
    {
        ex_set(e, EX_BAD_DATA,
                "function `assert` expects at most 3 arguments but %d "
                "were given", nargs);
        return e->nr;
    }

    if (ti_cq_scope(query, child->node, e) || ti_val_as_bool(query->rval))
        return e->nr;

    if (nargs == 1)
    {
        ex_set(e, EX_ASSERT_ERROR,
                "assertion statement `%.*s` has failed",
                (int) child->node->len, child->node->str);
        return e->nr;
    }

    child = child->next->next;
    ti_val_drop(query->rval);
    query->rval = NULL;

    if (ti_cq_scope(query, child->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `assert` expects argument 2 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead",
                ti_val_str(query->rval));
        return e->nr;
    }

    msg = (ti_raw_t *) query->rval;
    if (!strx_is_utf8n((const char *) msg->data, msg->n))
    {
        ex_set(e, EX_BAD_DATA,
                "function `assert` expects a message "
                "to have valid UTF8 encoding");
        return e->nr;
    }

    ex_setn(e, EX_ASSERT_ERROR, (const char *) msg->data, msg->n);

    if (nargs == 2)
        return e->nr;

    child = child->next->next;
    ti_val_drop(query->rval);
    query->rval = NULL;
    e->nr = 0;

    if (ti_cq_scope(query, child->node, e))
        return e->nr;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `assert` expects argument 3 to be of "
                "type `"TI_VAL_INT_S"` but got type `%s` instead",
                ti_val_str(query->rval));
        return e->nr;
    }

    code = (ti_vint_t *) query->rval;

    if (code->int_ < 1 || code->int_ > 32)
    {
        ex_set(e, EX_BAD_DATA,
                "function `assert` expects a custom error_code between "
                "1 and 32 but got %"PRId64" instead", code->int_);
        return e->nr;
    }

    e->nr = (ex_enum) code->int_;
    return e->nr;
}

static int cq__f_blob(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    int n_blobs = query->blobs ? query->blobs->n : 0;
    int64_t idx;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `blob` takes 1 argument but %d were given", n);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `blob` expects argument 1 to be of "
                "type `"TI_VAL_INT_S"` but got type `%s` instead",
                ti_val_str(query->rval));
        return e->nr;
    }

    idx = ((ti_vint_t *) query->rval)->int_;

    if (idx < 0)
        idx += n_blobs;

    if (idx < 0 || idx >= n_blobs)
    {
        ex_set(e, EX_INDEX_ERROR, "blob index out of range");
        return e->nr;
    }

    ti_val_drop(query->rval);
    query->rval = vec_get(query->blobs, idx);

    ti_incref(query->rval);
    return e->nr;
}

static int cq__f_del(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    cleri_node_t * name_nd;
    ti_task_t * task;
    ti_name_t * name;
    ti_raw_t * rname;
    ti_thing_t * thing = (ti_thing_t *) ti_query_val_pop(query);

    if (thing->tp != TI_VAL_THING)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `del`",
                ti_val_str((ti_val_t *) thing));
        goto done;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `del` takes 1 argument but %d were given", n);
        goto done;
    }

    if (!thing->id)
    {
        ex_set(e, EX_BAD_DATA,
                "function `del` can only be used on things with an id > 0; "
                "(things which are assigned automatically receive an id)");
        goto done;
    }

    if (ti_scope_in_use_thing(query->scope->prev, thing))
    {
        ex_set(e, EX_BAD_DATA,
                "cannot use `del` while thing "TI_THING_ID" is in use",
                thing->id);
        goto done;
    }

    name_nd = nd->children->node;

    if (ti_cq_scope(query, name_nd, e))
        goto done;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `del` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead",
                ti_val_str(query->rval));
        goto done;
    }

    rname = (ti_raw_t *) query->rval;
    name = ti_names_weak_get((const char *) rname->data, rname->n);

    if (!name || !ti_thing_del(thing, name))
    {
        if (ti_name_is_valid_strn((const char *) rname->data, rname->n))
        {
            ex_set(e, EX_INDEX_ERROR,
                    "thing "TI_THING_ID" has no property `%.*s`",
                    thing->id,
                    (int) rname->n, (const char *) rname->data);
        }
        else
        {
            ex_set(e, EX_BAD_DATA,
                    "function `del` expects argument 1 to be a valid name, "
                    "see "TI_DOCS"#names");
        }
        goto done;
    }

    task = ti_task_get_task(query->ev, thing, e);
    if (!task)
        goto done;

    if (ti_task_add_del(task, rname))
    {
        ex_set_alloc(e);
        goto done;
    }

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

done:
    ti_val_drop((ti_val_t *) thing);

    return e->nr;
}

static int cq__f_endswith(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * val = ti_query_val_pop(query);
    _Bool endswith;

    if (!ti_val_is_raw(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `endswith`",
                ti_val_str(val));
        goto done;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `endswith` takes 1 argument but %d were given", n);
        goto done;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        goto done;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `endswith` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead",
                ti_val_str(query->rval));
        goto done;
    }

    endswith = ti_raw_endswith((ti_raw_t *) val, (ti_raw_t *) query->rval);
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(endswith);

done:
    ti_val_drop(val);
    return e->nr;
}

static int cq__f_filter(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * retval = NULL;
    ti_closure_t * closure = NULL;
    ti_val_t * iterval = ti_query_val_pop(query);

    if (iterval->tp != TI_VAL_ARR && iterval->tp != TI_VAL_THING)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `filter`",
                ti_val_str(iterval));
        goto failed;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `filter` takes 1 argument but %d were given", n);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        goto failed;

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (closure->tp != TI_VAL_CLOSURE)
    {
        ex_set(e, EX_BAD_DATA,
                "function `filter` expects argument 1 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead",
                ti_val_str((ti_val_t *) closure));
        goto failed;
    }

    if (ti_scope_local_from_closure(query->scope, closure, e))
        goto failed;

    switch (iterval->tp)
    {
    case TI_VAL_THING:
    {
        ti_thing_t * thing;

        if (query->target->things->n >= query->target->quota->max_things)
        {
            ex_set(e, EX_MAX_QUOTA,
                    "maximum things quota of %zu has been reached, "
                    "see "TI_DOCS"#quotas", query->target->quota->max_things);
            goto failed;
        }

        thing = ti_thing_create(0, query->target->things);
        if (!thing)
            goto failed;

        retval = (ti_val_t *) thing;

        for (vec_each(((ti_thing_t *) iterval)->props, ti_prop_t, p))
        {
            if (ti_scope_polute_prop(query->scope, p))
                goto failed;

            if (cq__optscope(query, ti_closure_scope_nd(closure), e))
                goto failed;

            if (ti_val_as_bool(query->rval))
            {
                if (ti_thing_prop_set(thing, p->name, p->val))
                    goto failed;
                ti_incref(p->name);
                ti_incref(p->val);
            }

            ti_val_drop(query->rval);
            query->rval = NULL;
        }
        break;
    }
    case TI_VAL_ARR:
    {
        int64_t idx = 0;
        ti_varr_t * varr = ti_varr_create(((ti_varr_t *) iterval)->vec->n);
        if (!varr)
            goto failed;

        retval = (ti_val_t *) varr;

        for (vec_each(((ti_varr_t *) iterval)->vec, ti_val_t, v), ++idx)
        {
            if (ti_scope_polute_val(query->scope, v, idx))
                goto failed;

            if (cq__optscope(query, ti_closure_scope_nd(closure), e))
                goto failed;

            if (ti_val_as_bool(query->rval))
            {
                ti_incref(v);
                VEC_push(varr->vec, v);
            }

            ti_val_drop(query->rval);
            query->rval = NULL;

        }
        (void) vec_shrink(&varr->vec);
        break;
    }
    }

    assert (query->rval == NULL);
    query->rval = retval;

    goto done;

failed:
    ti_val_drop(retval);
    if (!e->nr)  /* all not set errors are allocation errors */
        ex_set_alloc(e);
done:
    ti_val_drop((ti_val_t *) closure);
    ti_val_drop(iterval);
    return e->nr;
}

static int cq__f_find(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    const int nargs = langdef_nd_n_function_params(nd);
    int64_t idx = 0;
    ti_closure_t * closure = NULL;
    ti_varr_t * varr = (ti_varr_t *) ti_query_val_pop(query);

    if (!ti_val_is_array((ti_val_t *) varr))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `find`",
                ti_val_str((ti_val_t *) varr));
        goto failed;
    }

    if (nargs < 1)
    {
        ex_set(e, EX_BAD_DATA,
                "function `find` requires at least 1 argument but 0 "
                "were given");
        goto failed;
    }

    if (nargs > 2)
    {
        ex_set(e, EX_BAD_DATA,
                "function `find` expects at most 2 arguments but %d "
                "were given", nargs);
        goto failed;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        goto failed;

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (closure->tp != TI_VAL_CLOSURE)
    {
        ex_set(e, EX_BAD_DATA,
                "function `find` expects argument 1 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead",
                ti_val_str((ti_val_t *) closure));
        goto failed;
    }

    if (ti_scope_local_from_closure(query->scope, closure, e))
        goto failed;

    for (vec_each(varr->vec, ti_val_t, v), ++idx)
    {
        _Bool found;

        if (ti_scope_polute_val(query->scope, v, idx))
            goto failed;

        if (cq__optscope(query, ti_closure_scope_nd(closure), e))
            goto failed;

        found = ti_val_as_bool(query->rval);
        ti_val_drop(query->rval);

        if (found)
        {
            query->rval = v;
            ti_incref(v);
            goto done;
        }

        query->rval = NULL;
    }

    assert (query->rval == NULL);
    if (nargs == 2)
    {
        /* lazy evaluation of the alternative value */
        if (ti_cq_scope(query, nd->children->next->next->node, e))
            goto failed;
    }
    else
    {
        query->rval = (ti_val_t *) ti_nil_get();
    }

    goto done;

failed:
    if (!e->nr)  /* all not set errors are allocation errors */
        ex_set_alloc(e);

done:
    ti_val_drop((ti_val_t *) closure);
    ti_val_drop((ti_val_t *) varr);

    return e->nr;
}

static int cq__f_findindex(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    size_t idx = 0;
    ti_varr_t * varr = (ti_varr_t *) ti_query_val_pop(query);
    ti_closure_t * closure = NULL;

    if (!ti_val_is_array((ti_val_t *) varr))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `findindex`",
                ti_val_str((ti_val_t *) varr));
        goto done;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `findindex` takes 1 argument but %d were given", n);
        goto done;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        goto done;

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (closure->tp != TI_VAL_CLOSURE)
    {
        ex_set(e, EX_BAD_DATA,
                "function `findindex` expects argument 1 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead",
                ti_val_str((ti_val_t *) closure));
        goto done;
    }

    if (ti_scope_local_from_closure(query->scope, closure, e))
        goto done;

    for (vec_each(varr->vec, ti_val_t, v), ++idx)
    {
        _Bool found;

        if (ti_scope_polute_val(query->scope, v, idx))
        {
            ex_set_alloc(e);
            goto done;
        }

        if (cq__optscope(query, ti_closure_scope_nd(closure), e))
            goto done;

        found = ti_val_as_bool(query->rval);
        ti_val_drop(query->rval);

        if (found)
        {
            query->rval = (ti_val_t *) ti_vint_create(idx);
            if (!query->rval)
                ex_set_alloc(e);

            goto done;
        }

        query->rval = NULL;
    }

    query->rval = (ti_val_t *) ti_nil_get();

done:
    ti_val_drop((ti_val_t *) closure);
    ti_val_drop((ti_val_t *) varr);

    return e->nr;
}

static int cq__f_hasprop(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_raw_t * rname;
    ti_name_t * name;
    ti_thing_t * thing = (ti_thing_t *) ti_query_val_pop(query);

    if (thing->tp != TI_VAL_THING)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `hasprop`",
                ti_val_str((ti_val_t *) thing));
        goto done;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `hasprop` takes 1 argument but %d were given", n);
        goto done;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        goto done;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `hasprop` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead",
                ti_val_str(query->rval));
        goto done;
    }

    rname = (ti_raw_t *) query->rval;
    name = ti_names_weak_get((const char *) rname->data, rname->n);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(
            name && ti_thing_prop_weak_get(thing, name));

done:
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}

static int cq__f_id(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_thing_t * thing = (ti_thing_t *) ti_query_val_pop(query);

    if (thing->tp != TI_VAL_THING)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `id`",
                ti_val_str((ti_val_t *) thing));
        goto done;
    }

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `id` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        goto done;
    }

    query->rval = (ti_val_t *) ti_vint_create((int64_t) thing->id);
    if (!query->rval)
        ex_set_alloc(e);

done:
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}

static int cq__f_indexof(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    size_t idx = 0;
    ti_varr_t * varr = (ti_varr_t *) ti_query_val_pop(query);

    if (!ti_val_is_list((ti_val_t *) varr))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `indexof`",
                ti_val_str((ti_val_t *) varr));
        goto done;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `indexof` takes 1 argument but %d were given", n);
        goto done;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        goto done;

    for (vec_each(varr->vec, ti_val_t, v), ++idx)
    {
        if (ti_opr_eq(v, query->rval))
        {
            ti_val_drop(query->rval);
            query->rval = (ti_val_t *) ti_vint_create(idx);
            if (!query->rval)
                ex_set_alloc(e);
            goto done;
        }
    }

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

done:
    ti_val_drop((ti_val_t *) varr);
    return e->nr;
}

static int cq__f_int(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `int` takes 1 argument but %d were given", n);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        return e->nr;

    return ti_val_convert_to_int(&query->rval, e);
}

static int cq__f_isarray(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool is_array;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `isarray` takes 1 argument but %d were given", n);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        return e->nr;

    is_array = ti_val_is_array(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_array);

    return e->nr;
}

static int cq__f_isascii(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool is_ascii;
    ti_raw_t * raw;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `isascii` takes 1 argument but %d were given", n);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    is_ascii = ti_val_is_raw(query->rval) &&
            strx_is_asciin((const char *) raw->data, raw->n);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_ascii);

    return e->nr;
}

static int cq__f_isbool(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool is_bool;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `isbool` takes 1 argument but %d were given", n);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        return e->nr;

    is_bool = ti_val_is_bool(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_bool);

    return e->nr;
}

static int cq__f_isfloat(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool is_float;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `isfloat` takes 1 argument but %d were given", n);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        return e->nr;

    is_float = ti_val_is_float(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_float);

    return e->nr;
}

static int cq__f_isinf(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool is_inf;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `isinf` takes 1 argument but %d were given", n);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        return e->nr;

    switch (query->rval->tp)
    {
    case TI_VAL_BOOL:
    case TI_VAL_INT:
        is_inf = false;
        break;
    case TI_VAL_FLOAT:
        is_inf = isinf(((ti_vfloat_t *) query->rval)->float_);
        break;
    default:
        ex_set(e, EX_BAD_DATA,
                "function `isinf` expects argument 1 to be of "
                "type `"TI_VAL_FLOAT_S"` but got type `%s` instead",
                ti_val_str(query->rval));
        return e->nr;
    }

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_inf);

    return e->nr;
}

static int cq__f_isint(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool is_isint;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `isint` takes 1 argument but %d were given", n);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        return e->nr;

    is_isint = ti_val_is_int(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_isint);

    return e->nr;
}

static int cq__f_islist(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool is_list;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `islist` takes 1 argument but %d were given", n);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        return e->nr;

    is_list = ti_val_is_list(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_list);

    return e->nr;
}

static int cq__f_isnan(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool is_nan;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `isnan` takes 1 argument but %d were given", n);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        return e->nr;

    switch (query->rval->tp)
    {
    case TI_VAL_BOOL:
    case TI_VAL_INT:
        is_nan = false;
        break;
    case TI_VAL_FLOAT:
        is_nan = isnan(((ti_vfloat_t *) query->rval)->float_);
        break;
    default:
        is_nan = true;
    }

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_nan);

    return e->nr;
}

static int cq__f_israw(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool is_raw;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `israw` takes 1 argument but %d were given", n);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        return e->nr;

    is_raw = ti_val_is_raw(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_raw);

    return e->nr;
}

static int cq__f_istuple(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool is_tuple;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `istuple` takes 1 argument but %d were given", n);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        return e->nr;

    is_tuple = ti_val_is_tuple(query->rval);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_tuple);

    return e->nr;
}

static int cq__f_isutf8(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool is_utf8;
    ti_raw_t * raw;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `isutf8` takes 1 argument but %d were given", n);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    is_utf8 = ti_val_is_raw(query->rval) &&
            strx_is_utf8n((const char *) raw->data, raw->n);

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_utf8);

    return e->nr;
}

static int cq__f_len(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * val = ti_query_val_pop(query);

    if (!ti_val_is_iterable(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `len`",
                ti_val_str(val));
        goto done;
    }

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `len` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        goto done;
    }

    query->rval = (ti_val_t *) ti_vint_create(
            (int64_t) ti_val_iterator_n(val));
    if (!query->rval)
        ex_set_alloc(e);

done:
    ti_val_drop(val);
    return e->nr;
}

static int cq__f_lower(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * val = ti_query_val_pop(query);

    if (!ti_val_is_raw(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `lower`",
                ti_val_str(val));
        goto done;
    }

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `lower` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        goto done;
    }

    query->rval = (ti_val_t *) ti_raw_lower((ti_raw_t *) val);
    if (!query->rval)
        ex_set_alloc(e);

done:
    ti_val_drop(val);
    return e->nr;
}

static int cq__f_map(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    size_t n;
    ti_varr_t * retvarr = NULL;
    ti_closure_t * closure = NULL;
    ti_val_t * iterval = ti_query_val_pop(query);

    if (iterval->tp != TI_VAL_ARR && iterval->tp != TI_VAL_THING)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `map`",
                ti_val_str(iterval));
        goto failed;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `map` takes 1 argument but %d were given", n);
        goto failed;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        goto failed;

    if (!ti_val_is_closure(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `map` expects argument 1 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead",
                ti_val_str(query->rval));
        goto failed;
    }

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    n = ti_val_iterator_n(iterval);

    if (ti_scope_local_from_closure(query->scope, closure, e))
        goto failed;

    retvarr = ti_varr_create(n);
    if (!retvarr)
        goto failed;

    switch (iterval->tp)
    {
    case TI_VAL_THING:
        for (vec_each(((ti_thing_t *) iterval)->props, ti_prop_t, p))
        {
            if (ti_scope_polute_prop(query->scope, p))
                goto failed;

            if (cq__optscope(query, ti_closure_scope_nd(closure), e))
                goto failed;

            if (ti_varr_append(retvarr, (void **) &query->rval, e))
                goto failed;

            query->rval = NULL;
        }
        break;
    case TI_VAL_ARR:
    {
        int64_t idx = 0;
        for (vec_each(((ti_varr_t *) iterval)->vec, ti_val_t, v), ++idx)
        {
            if (ti_scope_polute_val(query->scope, v, idx))
                goto failed;

            if (cq__optscope(query, ti_closure_scope_nd(closure), e))
                goto failed;

            if (ti_varr_append(retvarr, (void **) &query->rval, e))
                goto failed;

            query->rval = NULL;
        }
        break;
    }
    }

    assert (query->rval == NULL);
    assert (retvarr->vec->n == n);
    query->rval = (ti_val_t *) retvarr;

    goto done;

failed:
    ti_val_drop((ti_val_t *) retvarr);
    if (!e->nr)  /* all not set errors are allocation errors */
        ex_set_alloc(e);

done:
    ti_val_drop((ti_val_t *) closure);
    ti_val_drop(iterval);
    return e->nr;
}

static int cq__f_now(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `now` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    query->rval = (ti_val_t *) ti_vfloat_create(util_now());
    if (!query->rval)
        ex_set_alloc(e);

    return e->nr;
}

static int cq__f_pop(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool from_scope = !query->rval;
    ti_varr_t * varr = (ti_varr_t *) ti_query_val_pop(query);

    if (!ti_val_is_list((ti_val_t *) varr))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `pop`",
                ti_val_str((ti_val_t *) varr));
        goto done;
    }

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `pop` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        goto done;
    }

    if (from_scope && ti_scope_current_val_in_use(query->scope))
    {
        ex_set(e, EX_BAD_DATA,
                "cannot use function `pop` while the list is in use");
        goto done;
    }

    query->rval = vec_pop(varr->vec);

    if (!query->rval)
    {
        ex_set(e, EX_INDEX_ERROR, "pop from empty array");
        goto done;
    }

    (void) vec_shrink(&varr->vec);

    if (from_scope)
    {
        ti_task_t * task;
        assert (query->scope->thing);
        assert (query->scope->name);
        task = ti_task_get_task(query->ev, query->scope->thing, e);
        if (!task)
            goto done;

        if (ti_task_add_splice(
                task,
                query->scope->name,
                NULL,
                varr->vec->n,
                1,
                0))
            ex_set_alloc(e);
    }

done:
    ti_val_drop((ti_val_t *) varr);
    return e->nr;
}

static int cq__f_push(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    const int nargs = langdef_nd_n_function_params(nd);
    _Bool from_scope = !query->rval;
    cleri_children_t * child = nd->children;    /* first in argument list */
    uint32_t current_n, new_n;
    ti_varr_t * varr = (ti_varr_t *) ti_query_val_pop(query);

    if (!ti_val_is_list((ti_val_t *) varr))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `push`",
                ti_val_str((ti_val_t *) varr));
        goto done;
    }

    if (!nargs)
    {
        ex_set(e, EX_BAD_DATA,
                "function `push` requires at least 1 argument but 0 "
                "were given");
        goto done;
    }

    if (from_scope && ti_scope_current_val_in_use(query->scope))
    {
        ex_set(e, EX_BAD_DATA,
                "cannot use function `push` while the list is in use");
        goto done;
    }

    current_n = varr->vec->n;
    new_n = current_n + nargs;

    if (new_n >= query->target->quota->max_array_size)
    {
        ex_set(e, EX_MAX_QUOTA,
                "maximum array size quota of %zu has been reached, "
                "see "TI_DOCS"#quotas", query->target->quota->max_array_size);
        goto done;
    }

    if (vec_resize(&varr->vec, new_n))
    {
        ex_set_alloc(e);
        goto done;
    }

    assert (child);

    for (; child; child = child->next->next)
    {
        assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);

        if (ti_cq_scope(query, child->node, e))
            goto failed;

        if (ti_varr_append(varr, (void **) &query->rval, e))
            goto failed;

        query->rval = NULL;

        if (!child->next)
            break;
    }

    if (from_scope)
    {
        ti_task_t * task;
        assert (query->scope->thing);
        assert (query->scope->name);
        task = ti_task_get_task(query->ev, query->scope->thing, e);
        if (!task)
            goto failed;

        if (ti_task_add_splice(
                task,
                query->scope->name,
                varr,
                current_n,
                0,
                nargs))
            goto alloc_err;  /* we do not need to cleanup task, since the task
                                is added to `query->ev->tasks` */
    }

    assert (e->nr == 0);
    query->rval = (ti_val_t *) ti_vint_create((int64_t) varr->vec->n);
    if (query->rval)
        goto done;

alloc_err:
    ex_set_alloc(e);

failed:
    while (varr->vec->n > current_n)
    {
        ti_val_drop(vec_pop(varr->vec));
    }
    (void) vec_shrink(&varr->vec);

done:
    ti_val_drop((ti_val_t *) varr);
    return e->nr;
}

static int cq__f_refs(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    uint32_t ref;

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `refs` takes 1 argument but %d were given", n);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        return e->nr;

    ref = query->rval->ref;
    ti_val_drop(query->rval);

    query->rval = (ti_val_t *) ti_vint_create((int64_t) ref);
    if (!query->rval)
        ex_set_alloc(e);

    return e->nr;
}

static int cq__f_remove(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool from_scope = !query->rval;
    _Bool found;
    const int nargs = langdef_nd_n_function_params(nd);
    size_t idx = 0;
    ti_closure_t * closure = NULL;
    ti_varr_t * varr = (ti_varr_t *) ti_query_val_pop(query);

    if (!ti_val_is_list((ti_val_t *) varr))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `remove`",
                ti_val_str((ti_val_t *) varr));
        goto done;
    }

    if (nargs < 1)
    {
        ex_set(e, EX_BAD_DATA,
                "function `remove` requires at least 1 argument but 0 "
                "were given");
        goto done;
    }

    if (nargs > 2)
    {
        ex_set(e, EX_BAD_DATA,
                "function `remove` expects at most 2 arguments but %d "
                "were given", nargs);
        goto done;
    }

    if (from_scope && ti_scope_current_val_in_use(query->scope))
    {
        ex_set(e, EX_BAD_DATA,
                "cannot use function `remove` while the list is in use");
        goto done;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        goto done;

    closure = (ti_closure_t *) query->rval;
    query->rval = NULL;

    if (closure->tp != TI_VAL_CLOSURE)
    {
        ex_set(e, EX_BAD_DATA,
                "function `remove` expects argument 1 to be "
                "a `"TI_VAL_CLOSURE_S"` but got type `%s` instead",
                ti_val_str((ti_val_t *) closure));
        goto done;
    }

    if (ti_scope_local_from_closure(query->scope, closure, e))
        goto done;

    for (vec_each(varr->vec, ti_val_t, v), ++idx)
    {
        if (ti_scope_polute_val(query->scope, v, idx))
        {
            ex_set_alloc(e);
            goto done;
        }

        if (cq__optscope(query, ti_closure_scope_nd(closure), e))
            goto done;

        found = ti_val_as_bool(query->rval);
        ti_val_drop(query->rval);

        if (found)
        {
            query->rval = v;  /* we can move the reference here */
            (void) vec_remove(varr->vec, idx);

            if (from_scope)
            {
                ti_task_t * task;
                assert (query->scope->thing);
                assert (query->scope->name);
                task = ti_task_get_task(query->ev, query->scope->thing, e);
                if (!task)
                    goto done;

                if (ti_task_add_splice(
                        task,
                        query->scope->name,
                        NULL,
                        idx,
                        1,
                        0))
                    ex_set_alloc(e);
            }

            goto done;
        }
        query->rval = NULL;
    }

    assert (query->rval == NULL);

    if (nargs == 2)
    {
        /* lazy evaluation of the alternative value */
        (void) ti_cq_scope(query, nd->children->next->next->node, e);
    }
    else
    {
        query->rval = (ti_val_t *) ti_nil_get();
    }

done:
    ti_val_drop((ti_val_t *) closure);
    ti_val_drop((ti_val_t *) varr);
    return e->nr;
}

static int cq__f_rename(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    const int nargs = langdef_nd_n_function_params(nd);
    ti_thing_t * thing;
    cleri_node_t * from_nd, * to_nd;
    ti_raw_t * from_raw = NULL;
    ti_raw_t * to_raw;
    ti_name_t * from_name, * to_name;
    ti_task_t * task;

    thing = (ti_thing_t *) ti_query_val_pop(query);

    if (thing->tp != TI_VAL_THING)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `del`",
                ti_val_str((ti_val_t *) thing));
        goto done;
    }

    if (!thing->id)
    {
        ex_set(e, EX_BAD_DATA,
                "function `rename` requires a thing to be assigned, "
                "`del` should therefore be used in a separate statement");
        goto done;
    }

    if (nargs != 2)
    {
        ex_set(e, EX_BAD_DATA,
                "function `rename` expects 2 arguments but %d %s given",
                nargs, nargs == 1 ? "was" : "were");
        goto done;
    }

    if (ti_scope_in_use_thing(query->scope->prev, thing))
    {
        ex_set(e, EX_BAD_DATA,
                "cannot use `rename` while thing "TI_THING_ID" is in use",
                thing->id);
        goto done;
    }

    from_nd = nd
            ->children->node;
    to_nd = nd
            ->children->next->next->node;

    if (ti_cq_scope(query, from_nd, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `rename` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead",
                ti_val_str(query->rval));
        return e->nr;
    }

    from_raw = (ti_raw_t *) query->rval;
    query->rval = NULL;

    from_name = ti_names_weak_get((const char *) from_raw->data, from_raw->n);
    if (!from_name)
    {
        if (!ti_val_is_valid_name(query->rval))
        {
            ex_set(e, EX_BAD_DATA,
                    "function `rename` expects argument 1 to be a valid name, "
                    "see "TI_DOCS"#names");
        }
        else
        {
            ex_set(e, EX_INDEX_ERROR,
                    "thing "TI_THING_ID" has no property `%.*s`",
                    thing->id,
                    (int) from_raw->n, (const char *) from_raw->data);
        }
        goto done;
    }

    if (ti_cq_scope(query, to_nd, e))
        goto done;

    if (!ti_val_is_valid_name(query->rval))
    {
        if (ti_val_is_raw(query->rval))
        {
            ex_set(e, EX_BAD_DATA,
                    "function `rename` expects argument 2 to be a valid name, "
                    "see "TI_DOCS"#names");
        }
        else
        {
            ex_set(e, EX_BAD_DATA,
                    "function `rename` expects argument 2 to be of "
                    "type `"TI_VAL_RAW_S"` but got type `%s` instead",
                    ti_val_str(query->rval));
        }
        goto done;
    }

    to_raw = (ti_raw_t *) query->rval;
    to_name = ti_names_get((const char *) to_raw->data, to_raw->n);
    if (!to_name)
    {
        ex_set_alloc(e);
        goto done;
    }

    if (!ti_thing_rename(thing, from_name, to_name))
    {
        ti_name_drop(to_name);
        ex_set(e, EX_INDEX_ERROR,
                "thing "TI_THING_ID" has no property `%.*s`",
                thing->id,
                (int) from_raw->n, (const char *) from_raw->data);
        goto done;
    }

    task = ti_task_get_task(query->ev, thing, e);
    if (!task)
        goto done;

    if (ti_task_add_rename(task, from_raw, to_raw))
        goto alloc_err;

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    goto done;

alloc_err:
    ex_set_alloc(e);

done:
    ti_val_drop((ti_val_t *) from_raw);
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}

static int cq__f_ret(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `ret` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_nil_get();

    return e->nr;
}

static int cq__f_splice(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    _Bool from_scope = !query->rval;
    int32_t n, x, l;
    cleri_children_t * child = nd->children;    /* first in argument list */
    uint32_t current_n, new_n;
    int64_t i, c;
    ti_varr_t * retv;
    ti_varr_t * varr = (ti_varr_t *) ti_query_val_pop(query);

    if (!ti_val_is_list((ti_val_t *) varr))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `splice`",
                ti_val_str((ti_val_t *) varr));
        goto done;
    }

    n = langdef_nd_n_function_params(nd);
    if (n < 2)
    {
        ex_set(e, EX_BAD_DATA,
                "function `splice` requires at least 2 arguments "
                "but %d %s given",
                n, n == 1 ? "was" : "were");
        goto done;
    }

    if (from_scope && ti_scope_current_val_in_use(query->scope))
    {
        ex_set(e, EX_BAD_DATA,
                "cannot use function `splice` while the list is in use");
        goto done;
    }

    if (ti_cq_scope(query, child->node, e))
        goto done;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `splice` expects argument 1 to be of "
                "type `"TI_VAL_INT_S"` but got type `%s` instead",
                ti_val_str(query->rval));
        goto done;
    }

    i = ((ti_vint_t *) query->rval)->int_;
    ti_val_drop(query->rval);
    query->rval = NULL;
    child = child->next->next;

    if (ti_cq_scope(query, child->node, e))
        goto done;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `splice` expects argument 2 to be of "
                "type `"TI_VAL_INT_S"` but got type `%s` instead",
                ti_val_str(query->rval));
        goto done;
    }

    c = ((ti_vint_t *) query->rval)->int_;
    current_n = varr->vec->n;

    if (i < 0)
        i += current_n;

    i = i < 0 ? 0 : (i > current_n ? current_n : i);
    n -= 2;
    c = c < 0 ? 0 : (c > current_n - i ? current_n - i : c);
    new_n = current_n + n - c;

    if (new_n >= query->target->quota->max_array_size)
    {
        ex_set(e, EX_MAX_QUOTA,
                "maximum array size quota of %zu has been reached, "
                "see "TI_DOCS"#quotas", query->target->quota->max_array_size);
        goto done;
    }

    if (new_n > current_n && vec_resize(&varr->vec, new_n))
    {
        ex_set_alloc(e);
        goto done;
    }

    retv = ti_varr_create(c);
    if (!retv)
    {
        ex_set_alloc(e);
        goto done;
    }

    for (x = i, l = i + c; x < l; ++x)
        VEC_push(retv->vec, vec_get(varr->vec, x));

    memmove(
        varr->vec->data + i + n,
        varr->vec->data + i + c,
        (current_n - i - c) * sizeof(void*));

    varr->vec->n = i;

    for (x = 0; x < n; ++x)
    {
        child = child->next->next;
        assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);

        if (ti_cq_scope(query, child->node, e))
            goto failed;

        if (ti_varr_append(varr, (void **) &query->rval, e))
            goto failed;

        query->rval = NULL;
    }

    if (from_scope)
    {
        ti_task_t * task;

        task = ti_task_get_task(query->ev, query->scope->thing, e);
        if (!task)
            goto failed;

        if (ti_task_add_splice(
                task,
                query->scope->name,
                varr,
                i,
                c,
                n))
            goto alloc_err;  /* we do not need to cleanup task, since the task
                                is added to `query->ev->tasks` */
    }

    assert (e->nr == 0);

    /* required since query->rval may not be NULL */
    ti_val_drop(query->rval);

    query->rval = (ti_val_t *) retv;
    varr->vec->n = new_n;
    if (new_n < current_n)
        (void) vec_shrink(&varr->vec);
    goto done;

alloc_err:
    ex_set_alloc(e);

failed:
    while (x--)
        ti_val_drop(vec_pop(varr->vec));

    memmove(
        varr->vec->data + i + n,
        varr->vec->data + i + c,
        (current_n - i - c) * sizeof(void*));

    for (x = 0; x < c; ++x)
        VEC_push(varr->vec, vec_get(retv->vec, x));

    retv->vec->n = 0;
    ti_val_drop((ti_val_t *) retv);
    varr->vec->n = current_n;

done:
    ti_val_drop((ti_val_t *) varr);
    return e->nr;
}

static int cq__f_startswith(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * val = ti_query_val_pop(query);
    _Bool startswith;

    if (!ti_val_is_raw(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `startswith`",
                ti_val_str(val));
        goto done;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `startswith` takes 1 argument but %d were given", n);
        goto done;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        goto done;

    if (query->rval->tp != TI_VAL_RAW)
    {
        ex_set(e, EX_BAD_DATA,
                "function `startswith` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got type `%s` instead",
                ti_val_str(query->rval));
        goto done;
    }

    startswith = ti_raw_startswith((ti_raw_t *) val, (ti_raw_t *) query->rval);
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(startswith);

done:
    ti_val_drop(val);
    return e->nr;
}

static int cq__f_str(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `str` takes 1 argument but %d were given", n);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        return e->nr;

    if (ti_val_convert_to_str(&query->rval))
        ex_set_alloc(e);

    return e->nr;
}

static int cq__f_test(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * val = ti_query_val_pop(query);
    _Bool has_match;

    if (!ti_val_is_raw(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `test`",
                ti_val_str(val));
        goto done;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `test` takes 1 argument but %d were given", n);
        goto done;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        goto done;

    if (!ti_val_is_regex(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `test` expects argument 1 to be "
                "of type `"TI_VAL_REGEX_S"` but got type `%s` instead",
                ti_val_str(query->rval));
        goto done;
    }

    has_match = ti_regex_test((ti_regex_t *) query->rval, (ti_raw_t *) val);
    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(has_match);

done:
    ti_val_drop(val);
    return e->nr;
}

static int cq__f_t(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    const int nargs = langdef_nd_n_function_params(nd);
    ti_varr_t * varr = NULL;
    cleri_children_t * child = nd->children;    /* first in argument list */

    if (!nargs)
    {
        ex_set(e, EX_BAD_DATA,
                "function `t` requires at least 1 argument but 0 "
                "were given");
        return e->nr;
    }

    assert (child);

    for (int arg = 1; child; child = child->next->next, ++arg)
    {
        ti_thing_t * thing;
        assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);

        if (ti_cq_scope(query, child->node, e))
            goto failed;

        if (!ti_val_is_int(query->rval))
        {
            ex_set(e, EX_BAD_DATA,
                "function `t` only accepts type `"TI_VAL_INT_S"` arguments, "
                "but argument %d is of type `%s`",
                arg, ti_val_str(query->rval));
            goto failed;
        }

        thing = ti_collection_thing_by_id(
                query->target,
                ((ti_vint_t *) query->rval)->int_);
        if (!thing)
        {
            ex_set(e, EX_INDEX_ERROR,
                    "collection `%.*s` has no `thing` with id `%"PRId64"`",
                    (int) query->target->name->n,
                    (char *) query->target->name->data,
                    ((ti_vint_t *) query->rval)->int_);
            goto failed;
        }

        ti_val_drop(query->rval);

        if (arg == 1)
        {
            if (!child->next)
            {
                assert (nargs == 1);
                ti_incref(thing);
                query->rval = (ti_val_t *) thing;
                if (ti_scope_push_thing(&query->scope, thing))
                    ex_set_alloc(e);

                return e->nr; /* only one thing, no array */
            }

            varr = ti_varr_create(nargs);
            if (!varr)
            {
                ex_set_alloc(e);
                return e->nr;
            }
        }

        query->rval = NULL;
        ti_incref(thing);
        VEC_push(varr->vec, thing);

        if (!child->next)
            break;
    }

    assert (varr);
    assert (varr->vec->n >= 1);

    query->rval = (ti_val_t *) varr;
    return 0;

failed:
    ti_val_drop((ti_val_t *) varr);
    return e->nr;
}

static int cq__f_try(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    cleri_children_t * child = nd->children;    /* first in argument list */
    const int nargs = langdef_nd_n_function_params(nd);
    ex_enum errnr;
    cleri_node_t * alt_node;

    if (nargs < 1)
    {
        ex_set(e, EX_BAD_DATA,
                "function `try` requires at least 1 argument but 0 "
                "were given");
        return e->nr;
    }

    errnr = ti_cq_scope(query, child->node, e);

    if (errnr > -100)
        return errnr;   /* return when successful or internal errors */

    ti_val_drop(query->rval);
    e->nr = 0;

    if (nargs == 1)
    {
        query->rval = (ti_val_t *) ti_nil_get();
        return 0;
    }

    query->rval = NULL;
    child = child->next->next;
    alt_node = child->node;

    if (nargs == 2)
        goto returnalt;

    for (child = child->next->next; child; child = child->next->next)
    {
        if (ti_cq_scope(query, child->node, e))
            return e->nr;

        if (ti_val_convert_to_errnr(&query->rval, e))
            return e->nr;

        assert (query->rval->tp == TI_VAL_INT);

        if ((ex_enum) ((ti_vint_t * ) query->rval)->int_ == errnr)
        {
            ti_val_drop(query->rval);
            query->rval = NULL;
            goto returnalt;  /* catch the error */
        }

        if (!child->next)
            break;
    }

    e->nr = errnr;
    return e->nr;

returnalt:
    /* lazy evaluation of the alternative return value */
    (void) ti_cq_scope(query, alt_node, e);
    return e->nr;
}

static int cq__f_upper(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * val = ti_query_val_pop(query);

    if (!ti_val_is_raw(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `upper`",
                ti_val_str(val));
        goto done;
    }

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `upper` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        goto done;
    }

    query->rval = (ti_val_t *) ti_raw_upper((ti_raw_t *) val);
    if (!query->rval)
        ex_set_alloc(e);

done:
    ti_val_drop(val);
    return e->nr;
}

static int cq__function(
        ti_query_t * query,
        cleri_node_t * nd,
        _Bool is_scope,             /* scope or chain */
        ex_t * e)
{
    assert (e->nr == 0);
    assert (langdef_nd_is_function(nd));

    cleri_node_t * fname, * params;

    fname = nd                      /* sequence */
            ->children->node        /* choice */
            ->children->node;       /* keyword or name node */

    params = nd                             /* sequence */
            ->children->next->next->node;   /* list of scope (arguments) */


    switch (fname->cl_obj->gid)
    {
    case CLERI_GID_F_ASSERT:
        if (is_scope)
            return cq__f_assert(query, params, e);
        break;
    case CLERI_GID_F_BLOB:
        if (is_scope)
            return cq__f_blob(query, params, e);
        break;
    case CLERI_GID_F_ENDSWITH:
        return cq__f_endswith(query, params, e);
    case CLERI_GID_F_DEL:
        return cq__f_del(query, params, e);
    case CLERI_GID_F_FILTER:
        return cq__f_filter(query, params, e);
    case CLERI_GID_F_FIND:
        return cq__f_find(query, params, e);
    case CLERI_GID_F_FINDINDEX:
        return cq__f_findindex(query, params, e);
    case CLERI_GID_F_HASPROP:
        return cq__f_hasprop(query, params, e);
    case CLERI_GID_F_ID:
        return cq__f_id(query, params, e);
    case CLERI_GID_F_INDEXOF:
        return cq__f_indexof(query, params, e);
    case CLERI_GID_F_INT:
        if (is_scope)
            return cq__f_int(query, params, e);
        break;
    case CLERI_GID_F_ISARRAY:
        if (is_scope)
            return cq__f_isarray(query, params, e);
        break;
    case CLERI_GID_F_ISASCII:
        if (is_scope)
            return cq__f_isascii(query, params, e);
        break;
    case CLERI_GID_F_ISBOOL:
        if (is_scope)
            return cq__f_isbool(query, params, e);
        break;
    case CLERI_GID_F_ISFLOAT:
        if (is_scope)
            return cq__f_isfloat(query, params, e);
        break;
    case CLERI_GID_F_ISINF:
        if (is_scope)
            return cq__f_isinf(query, params, e);
        break;
    case CLERI_GID_F_ISINT:
        if (is_scope)
            return cq__f_isint(query, params, e);
        break;
    case CLERI_GID_F_ISLIST:
        if (is_scope)
            return cq__f_islist(query, params, e);
        break;
    case CLERI_GID_F_ISNAN:
        if (is_scope)
            return cq__f_isnan(query, params, e);
        break;
    case CLERI_GID_F_ISRAW:
        if (is_scope)
            return cq__f_israw(query, params, e);
        break;
    case CLERI_GID_F_ISTUPLE:
        if (is_scope)
            return cq__f_istuple(query, params, e);
        break;
    case CLERI_GID_F_ISSTR:
    case CLERI_GID_F_ISUTF8:
        if (is_scope)
            return cq__f_isutf8(query, params, e);
        break;
    case CLERI_GID_F_LEN:
        return cq__f_len(query, params, e);
    case CLERI_GID_F_LOWER:
        return cq__f_lower(query, params, e);
    case CLERI_GID_F_MAP:
        return cq__f_map(query, params, e);
    case CLERI_GID_F_NOW:
        if (is_scope)
            return cq__f_now(query, params, e);
        break;
    case CLERI_GID_F_POP:
        return cq__f_pop(query, params, e);
    case CLERI_GID_F_PUSH:
        return cq__f_push(query, params, e);
    case CLERI_GID_F_REFS:
        if (is_scope)
            return cq__f_refs(query, params, e);
        break;
    case CLERI_GID_F_REMOVE:
        return cq__f_remove(query, params, e);
    case CLERI_GID_F_RENAME:
        return cq__f_rename(query, params, e);
    case CLERI_GID_F_RET:
        return cq__f_ret(query, params, e);
    case CLERI_GID_F_SPLICE:
        return cq__f_splice(query, params, e);
    case CLERI_GID_F_STARTSWITH:
        return cq__f_startswith(query, params, e);
    case CLERI_GID_F_STR:
        if (is_scope)
            return cq__f_str(query, params, e);
        break;
    case CLERI_GID_F_T:
        if (is_scope)
            return cq__f_t(query, params, e);
        break;
    case CLERI_GID_F_TEST:
        return cq__f_test(query, params, e);
    case CLERI_GID_F_TRY:
        if (is_scope)
            return cq__f_try(query, params, e);
        break;
    case CLERI_GID_F_UPPER:
        return cq__f_upper(query, params, e);
    }

    /* set error */
    if (is_scope)
    {
        ex_set(e, EX_INDEX_ERROR,
                "unknown function `%.*s`",
                fname->len,
                fname->str);
    }
    else
    {
        ti_val_t * val = ti_query_val_pop(query);
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `%.*s`",
                ti_val_str(val),
                fname->len,
                fname->str);
        ti_val_drop(val);
    }

    return e->nr;
}

static int cq__index(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_INDEX);
    assert (nd->children);

    cleri_children_t * child;
    cleri_node_t * node;
    ti_val_t * val = ti_query_val_pop(query);
    int64_t idx;
    ssize_t n;

    /* multiple indexes are possible, for example x[0][0] */
    for (child = nd->children; child; child = child->next)
    {
        node = child->node              /* sequence  [ int ]  */
            ->children->next->node;     /* scope */

        if (ti_cq_scope(query, node, e))
            return e->nr;

        if (!ti_val_is_int(query->rval))
        {
            ex_set(e, EX_BAD_DATA,
                    "expecting an index to be of "
                    "type `"TI_VAL_INT_S"` but got type `%s` instead",
                    ti_val_str(query->rval));
            return e->nr;
        }

        idx = ((ti_vint_t * ) query->rval)->int_;

        switch (val->tp)
        {
        case TI_VAL_RAW:
            n = ((ti_raw_t *) val)->n;
            break;
        case TI_VAL_ARR:
            n = ((ti_varr_t *) val)->vec->n;
            break;
        default:
            ex_set(e, EX_BAD_DATA, "type `%s` is not indexable",
                    ti_val_str(val));
            goto done;
        }

        if (idx < 0)
            idx += n;

        if (idx < 0 || idx >= n)
        {
            ex_set(e, EX_INDEX_ERROR, "index out of range");
            goto done;
        }

        ti_val_drop(query->rval);

        switch (val->tp)
        {
        case TI_VAL_RAW:
            query->rval = (ti_val_t *) ti_vint_create(
                    (int64_t) ((ti_raw_t *) val)->data[idx]);
            if (!query->rval)
            {
                ex_set_alloc(e);
                goto done;
            }
            break;
        case TI_VAL_ARR:
            query->rval = vec_get(((ti_varr_t *) val)->vec, idx);
            ti_incref(query->rval);
            break;
        default:
            assert (0);
            ex_set_internal(e);
            goto done;
        }

        if (query->rval->tp == TI_VAL_THING &&
            ti_scope_push_thing(&query->scope, (ti_thing_t *) query->rval))
        {
            ex_set_alloc(e);
            goto done;
        }

        if (!child->next)
            break;

        ti_val_drop(val);
        val = query->rval;
        query->rval = NULL;
    }

done:
    ti_val_drop(val);
    return e->nr;
}

static int cq__operations(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    uint32_t gid;
    ti_val_t * a_val = NULL;
    assert( nd->cl_obj->tp == CLERI_TP_RULE ||
            nd->cl_obj->tp == CLERI_TP_PRIO ||
            nd->cl_obj->tp == CLERI_TP_THIS);

    nd = nd->cl_obj->tp == CLERI_TP_PRIO ?
            nd                          /* prio */
            ->children->node :          /* compare sequence */
            nd                          /* rule/this */
            ->children->node            /* prio */
            ->children->node;           /* compare sequence */

    assert (nd->cl_obj->tp == CLERI_TP_SEQUENCE);
    assert (query->rval == NULL);

    if (nd->cl_obj->gid == CLERI_GID_SCOPE)
        return ti_cq_scope(query, nd, e);

    gid = nd->children->next->node->children->node->cl_obj->gid;

    switch (gid)
    {
    case CLERI_GID_OPR0_MUL_DIV_MOD:
    case CLERI_GID_OPR1_ADD_SUB:
    case CLERI_GID_OPR2_BITWISE_AND:
    case CLERI_GID_OPR3_BITWISE_XOR:
    case CLERI_GID_OPR4_BITWISE_OR:
    case CLERI_GID_OPR5_COMPARE:
        if (cq__operations(query, nd->children->node, e))
            return e->nr;
        a_val = query->rval;
        query->rval = NULL;
        if (cq__operations(query, nd->children->next->next->node, e))
            break;
        (void) ti_opr_a_to_b(a_val, nd->children->next->node, &query->rval, e);
        break;

    case CLERI_GID_OPR6_CMP_AND:
        if (    cq__operations(query, nd->children->node, e) ||
                !ti_val_as_bool(query->rval))
            return e->nr;

        ti_val_drop(query->rval);
        query->rval = NULL;
        return cq__operations(query, nd->children->next->next->node, e);

    case CLERI_GID_OPR7_CMP_OR:
        if (    cq__operations(query, nd->children->node, e) ||
                ti_val_as_bool(query->rval))
            return e->nr;

        ti_val_drop(query->rval);
        query->rval = NULL;
        return cq__operations(query, nd->children->next->next->node, e);

    default:
        assert (0);
    }

    ti_val_drop(a_val);
    return e->nr;
}

static int cq__optscope(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (query->rval == NULL);
    if (!nd)
    {
        query->rval = (ti_val_t *) ti_nil_get();
        return e->nr;
    }
    return ti_cq_scope(query, nd, e);
}

static int cq__primitives(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_PRIMITIVES);
    assert (!e->nr);

    cleri_node_t * node = nd            /* choice */
            ->children->node;           /* false, nil, true, undefined,
                                           int, float, string */

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_T_FALSE:
        query->rval = (ti_val_t *) ti_vbool_get(false);
        break;
    case CLERI_GID_T_FLOAT:
        if (!node->data)
        {
            node->data = ti_vfloat_create(strx_to_double(node->str));
            if (!node->data)
            {
                ex_set_alloc(e);
                return e->nr;
            }
            assert (vec_space(query->nd_cache));
            VEC_push(query->nd_cache, node->data);
        }
        query->rval = node->data;
        ti_incref(query->rval);
        break;
    case CLERI_GID_T_INT:
        if (!node->data)
        {
            node->data = ti_vint_create(strx_to_int64(node->str));
            if (!node->data)
            {
                ex_set_alloc(e);
                return e->nr;
            }
            if (errno == ERANGE)
            {
                ex_set(e, EX_OVERFLOW, "integer overflow");
                return e->nr;
            }
            assert (vec_space(query->nd_cache));
            VEC_push(query->nd_cache, node->data);
        }
        query->rval = node->data;
        ti_incref(query->rval);
        break;
    case CLERI_GID_T_NIL:
        query->rval = (ti_val_t *) ti_nil_get();
        break;
    case CLERI_GID_T_REGEX:
        if (!node->data)
        {
            node->data = ti_regex_from_strn(node->str, node->len, e);
            if (!node->data)
                return e->nr;
            assert (vec_space(query->nd_cache));
            VEC_push(query->nd_cache, node->data);
        }
        query->rval = node->data;
        ti_incref(query->rval);
        break;
    case CLERI_GID_T_STRING:
        if (!node->data)
        {
            node->data = ti_raw_from_ti_string(node->str, node->len);
            if (!node->data)
            {
                ex_set_alloc(e);
                return e->nr;
            }
            assert (vec_space(query->nd_cache));
            VEC_push(query->nd_cache, node->data);
        }
        query->rval = node->data;
        ti_incref(query->rval);
        break;
    case CLERI_GID_T_TRUE:
        query->rval = (ti_val_t *) ti_vbool_get(true);
        break;
    }
    return e->nr;
}

/* changes scope->name and/or scope->thing */
static int cq__scope_name(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_NAME);
    assert (ti_name_is_valid_strn(nd->str, nd->len));
    assert (ti_scope_is_thing(query->scope));

    ti_name_t * name;
    ti_val_t * val;

    name = ti_names_weak_get(nd->str, nd->len);
    val = name ? ti_scope_find_local_val(query->scope, name) : NULL;

    if (!val && name)
        val = ti_thing_prop_weak_get(query->target->root, name);

    if (!val)
    {
        ex_set(e, EX_INDEX_ERROR,
                "`%.*s` is undefined",
                (int) nd->len, nd->str);
        return e->nr;
    }

    // TODO: why was this code here? and why not in chain_name?
//    ti_val_drop(query->rval);
//    query->rval = NULL;

    if (ti_scope_push_name(&query->scope, name, val))
        ex_set_alloc(e);

    return e->nr;
}

static int cq__scope_thing(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    /*
     * Sequence('{', List(Sequence(name, ':', scope)), '}')
     */
    assert (e->nr == 0);

    ti_thing_t * thing;
    cleri_children_t * child;
    size_t max_props = query->target->quota->max_props;
    size_t max_things = query->target->quota->max_things;

    if (query->target->things->n >= max_things)
    {
        ex_set(e, EX_MAX_QUOTA,
                "maximum things quota of %zu has been reached, "
                "see "TI_DOCS"#quotas", max_things);
        return e->nr;
    }

    thing = ti_thing_create(0, query->target->things);
    if (!thing)
        goto alloc_err;

    child = nd                                  /* sequence */
            ->children->next->node              /* list */
            ->children;                         /* list items */

    for (; child; child = child->next->next)
    {
        cleri_node_t * name_nd;
        cleri_node_t * scope;
        ti_name_t * name;

        if (thing->props->n == max_props)
        {
            ex_set(e, EX_MAX_QUOTA,
                    "maximum properties quota of %zu has been reached, "
                    "see "TI_DOCS"#quotas", max_props);
            goto err;
        }

        name_nd = child->node                       /* sequence */
                ->children->node;                   /* name */

        scope = child->node                         /* sequence */
                ->children->next->next->node;       /* scope */

        if (ti_cq_scope(query, scope, e))
            goto err;

        name = ti_names_get(name_nd->str, name_nd->len);
        if (!name)
            goto alloc_err;

        if (ti_thing_prop_set(thing, name, query->rval))
        {
            ti_name_drop(name);
            goto alloc_err;
        }

        query->rval = NULL;

        if (!child->next)
            break;
    }

    if (ti_scope_push_thing(&query->scope, thing))
        goto alloc_err;

    query->rval = (ti_val_t *) thing;
    return 0;

alloc_err:
    ex_set_alloc(e);
err:
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}


/* changes scope->name and/or scope->thing */
static int cq__tmp(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_TMP);
    assert (nd->len >= 2);
    assert (ti_name_is_valid_strn(nd->str + 1, nd->len - 1));
    assert (ti_scope_is_thing(query->scope));

    ti_name_t * name;
    ti_prop_t * prop;

    name = ti_names_weak_get(nd->str, nd->len);
    prop = name ? ti_query_tmpprop_get(query, name) : NULL;

    if (!prop)
    {
        ex_set(e, EX_INDEX_ERROR,
                "temporary variable `%.*s` is undefined",
                (int) nd->len, nd->str);
        return e->nr;
    }

    if (ti_scope_push_name(&query->scope, name, prop->val))
        ex_set_alloc(e);

    ti_val_drop(query->rval);
    query->rval = NULL;

    return e->nr;
}

static int cq__tmp_assign(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_TMP_ASSIGN);

    ti_thing_t * thing;
    ti_name_t * name;
    ti_prop_t * prop = NULL;     /* assign to prevent warning */
    cleri_node_t * name_nd = nd                 /* sequence */
            ->children->node;                   /* name */
    cleri_node_t * assign_nd = nd               /* sequence */
            ->children->next->node;             /* assign tokens */
    cleri_node_t * scope_nd = nd                /* sequence */
            ->children->next->next->node;       /* scope */

    thing = (ti_thing_t *) ti_query_val_pop(query);
    assert (thing->tp == TI_VAL_THING);

    name = ti_names_get(name_nd->str, name_nd->len);
    if (!name)
        goto alloc_err;

    if (assign_nd->len == 1)
        prop = NULL;
    else
    {
        /* apply to existing variable */
        assert (assign_nd->len == 2);
        assert (assign_nd->str[1] == '=');
        prop = ti_query_tmpprop_get(query, name);
        if (!prop)
        {
            ex_set(e, EX_INDEX_ERROR,
                    "`%.*s` is undefined", (int) name->n, name->str);
            goto failed;
        }
        ti_decref(name);  /* we already have a reference to name */
    }

    /* should also work in chain because then scope->local must be NULL */
    if (    ti_scope_has_local_name(query->scope, name) ||
            ti_scope_in_use_name(query->scope, thing, name))
    {
        ex_set(e, EX_BAD_DATA,
            "cannot assign a new value to `%.*s` while the variable is in use",
            (int) name->n, name->str);
        goto failed;
    }

    if (ti_cq_scope(query, scope_nd, e))
        goto failed;

    assert (query->rval);

    if (assign_nd->len == 2)
    {
        assert (prop);
        if (ti_opr_a_to_b(prop->val, assign_nd, &query->rval, e))
            goto failed;
    }

    if (prop)
    {
        ti_val_drop(prop->val);
        prop->val = query->rval;
    }
    else
    {
        prop = ti_prop_create(name, query->rval);
        if (!prop)
            goto alloc_err;

        if (query->tmpvars)
        {
            if (vec_push(&query->tmpvars, prop))
                goto alloc_err_with_prop;
        }
        else
        {
            query->tmpvars = vec_new(1);
            if (!query->tmpvars)
                goto alloc_err_with_prop;
            VEC_push(query->tmpvars, prop);
        }
    }

    ti_incref(query->rval);

    goto done;

alloc_err_with_prop:
    /* prop->name will be dropped and prop->val is still on query->rval */
    free(prop);

alloc_err:
    ex_set_alloc(e);

failed:
    ti_name_drop(name);

done:
    ti_val_drop((ti_val_t *) thing);
    return e->nr;
}
