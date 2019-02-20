/*
 * ti/cq.c
 */
#include <assert.h>
#include <langdef/langdef.h>
#include <langdef/nd.h>
#include <math.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/arrow.h>
#include <ti/cq.h>
#include <ti/names.h>
#include <ti/nil.h>
#include <ti/opr.h>
#include <ti/prop.h>
#include <ti/regex.h>
#include <ti/vbool.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <util/query.h>
#include <util/strx.h>
#include <util/util.h>

static int cq__array(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__arrow(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__assignment(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__chain(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__chain_name(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_blob(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_del(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_endswith(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_filter(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_find(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_get(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_hasprop(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_id(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_int(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_isarray(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_isinf(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_islist(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_isnan(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_len(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_lower(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_map(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_now(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_pop(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_push(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_rename(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_refs(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_ret(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_set(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_splice(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_startswith(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_str(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_test(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_t(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_try(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_unset(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_upper(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__function(
        ti_query_t * query,
        cleri_node_t * nd,
        _Bool is_scope,             /* scope or chain */
        ex_t * e);
static int cq__index(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__operations(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__primitives(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__scope_name(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__scope_thing(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__tmp(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__tmp_assign(ti_query_t * query, cleri_node_t * nd, ex_t * e);


int ti_cq_scope(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_SCOPE);

    int nots = 0;
    cleri_node_t * node;
    cleri_children_t * nchild, * child = nd /* sequence */
                    ->children;             /* first child, not */
    ti_scope_t * current_scope, * scope;

    for (nchild = child->node->children; nchild; nchild = nchild->next)
        ++nots;

    child = child->next;
    node = child->node                      /* choice */
            ->children->node;               /* primitives, function,
                                               assignment, name, thing,
                                               array, compare, arrow */
    current_scope = query->scope;
    scope = ti_scope_enter(current_scope, query->target->root);
    if (!scope)
    {
        ex_set_alloc(e);
        return e->nr;
    }
    query->scope = scope;

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_ARRAY:
        if (cq__array(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_ARROW:
        if (cq__arrow(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_ASSIGNMENT:
        if (cq__assignment(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_FUNCTION:
        if (cq__function(query, node, true, e))
            return e->nr;
        break;
    case CLERI_GID_NAME:
        if (cq__scope_name(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_OPERATIONS:
        /* skip the sequence , jump to the priority list */
        if (cq__operations(query, node->children->next->node, e))
            return e->nr;

        if (node->children->next->next->next)               /* optional */
        {
            node = node->children->next->next->next->node   /* choice */
                   ->children->node;                        /* sequence */
            if (ti_cq_scope(
                    query,
                    ti_val_as_bool(query->rval)
                        ? node->children->next->node        /* scope, true */
                        : node->children->next->next->next->node, /* false */
                    e))
                return e->nr;
        }
        break;
    case CLERI_GID_PRIMITIVES:
        if (cq__primitives(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_THING:
        if (cq__scope_thing(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_TMP:
        if (cq__tmp(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_TMP_ASSIGN:
        if (cq__tmp_assign(query, node, e))
            return e->nr;
        break;

    default:
        assert (0);  /* all possible should be handled */
        return -1;
    }

    child = child->next;

    assert (child);

    node = child->node;
    assert (node->cl_obj->gid == CLERI_GID_INDEX);

    if (node->children && cq__index(query, node, e))
        return e->nr;

    child = child->next;
    if (!child)
        goto finish;

    node = child->node              /* optional */
            ->children->node;       /* chain */

    assert (node->cl_obj->gid == CLERI_GID_CHAIN);

    if (cq__chain(query, node, e))
        goto done;

finish:

    if (!query->rval)
    {
        query->rval = ti_scope_global_to_val(query->scope);
        if (!query->rval)
        {
            ex_set_alloc(e);
            goto done;
        }
    }

    if (nots)
    {
        _Bool b = ti_val_as_bool(query->rval);
        ti_val_drop(query->rval);
        query->rval = (ti_val_t *) ti_vbool_get((nots & 1) ^ b);
    }

done:
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

        if (ti_varr_append(varr, query->rval, e))
            goto failed;

        ti_decref(query->rval);
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


static int cq__arrow(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_ARROW);

    query->rval = (ti_val_t *) ti_arrow_from_node(nd);
    if (!query->rval)
        ex_set_alloc(e);

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

    assert (query->rval);

    if (assign_nd->len == 2)
    {
        assert (left_val);
        if (ti_opr_a_to_b(left_val, assign_nd, &query->rval, e))
            goto fail1;
    }
    else if (ti_val_make_assignable(query->rval, e))
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

    query->rval = (ti_val_t *) ti_nil_get();

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

    assert (child);

    node = child->node;
    assert (node->cl_obj->gid == CLERI_GID_INDEX);

    if (node->children && cq__index(query, node, e))
        return e->nr;

    child = child->next;
    if (!child)
        goto finish;

    node = child->node              /* optional */
            ->children->node;       /* chain */

    assert (node->cl_obj->gid == CLERI_GID_CHAIN);

    (void) cq__chain(query, node, e);

finish:
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
                query_tp_str(query),
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

    if (query->rval->tp != TI_VAL_INT)
    {
        ex_set(e, EX_BAD_DATA,
                "function `blob` expects argument 1 to be of "
                "type `"TI_VAL_INT_S"` but got `%s`", ti_val_str(query->rval));
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

    assert (query->rval);

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

    if (!thing->id)
    {
        ex_set(e, EX_BAD_DATA,
                "function `del` requires a thing to be assigned, "
                "`del` should therefore be used in a separate statement");
        goto done;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `del` takes 1 argument but %d were given", n);
        goto done;
    }

    if (ti_scope_in_use_thing(query->scope->prev, thing))
    {
        ex_set(e, EX_BAD_DATA,
                "cannot use `del` while thing "TI_THING_ID" is in use");
        goto done;
    }

    name_nd = nd->children->node;

    if (ti_cq_scope(query, name_nd, e))
        goto done;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `del` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got `%s`",
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

    if (val->tp != TI_VAL_RAW)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `endswith`",
                query_tp_str(query));
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

    if (query->rval->tp != TI_VAL_RAW)
    {
        ex_set(e, EX_BAD_DATA,
                "function `endswith` expects argument 1 to be of "
                "type `"TI_VAL_RAW_S"` but got `%s`",
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
    ti_arrow_t * arrow = NULL;
    ti_val_t * iterval = ti_query_val_pop(query);

    if (iterval->tp != TI_VAL_ARR && iterval->tp != TI_VAL_THING)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `filter`",
                query_tp_str(query));
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

    arrow = (ti_arrow_t *) query->rval;
    query->rval = NULL;

    if (arrow->tp != TI_VAL_ARROW)
    {
        ex_set(e, EX_BAD_DATA,
                "function `filter` expects an `"TI_VAL_ARROW_S"` "
                "but got type `%s` instead",
                ti_val_str(query->rval));
        goto failed;
    }

    if (ti_scope_local_from_node(query->scope, arrow->node, e))
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
            size_t n = 0;
            for (vec_each(query->scope->local, ti_prop_t, prop), ++n)
            {
                switch (n)
                {
                case 0:
                    /* TODO: Or, check for errors, or change name_t */
                    prop->val = (ti_val_t *) ti_raw_from_strn(
                            p->name->str, p->name->n);
                    break;
                case 1:
                    prop->val = p->val;
                    ti_incref(p->val);
                    break;
                default:
                    prop->val = (ti_val_t *) ti_nil_get();
                }
            }

            if (ti_cq_scope(query, ti_arrow_scope_nd(arrow), e))
                goto failed;

            if (ti_val_as_bool(query->rval))
            {
                ti_incref(p->name);
                if (ti_thing_prop_set(thing, p->name, p->val))
                    goto failed;
            }

            query_rval_weak_destroy(query);
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
            size_t n = 0;
            for (vec_each(query->scope->local, ti_prop_t, prop), ++n)
            {
                switch (n)
                {
                case 0:
                    prop->val = v;
                    break;
                case 1:
                    prop->val = (ti_val_t *) ti_vint_create(idx);
                    break;
                default:
                    prop->val = (ti_val_t *) ti_nil_get();
                }
            }

            if (ti_cq_scope(query, ti_arrow_scope_nd(arrow), e))
                goto failed;

            if (ti_val_as_bool(query->rval))
            {
                ti_incref(v);
                VEC_push(varr, v);
            }
            query_rval_destroy(query);
        }
        (void) vec_shrink(&varr->vec);
        break;
    }
    }

    assert (query->rval == NULL);
    query->rval = retval;

    goto done;

failed:
    if (!e->nr)  /* all not set errors are allocation errors */
        ex_set_alloc(e);

done:
    ti_val_drop(iterval);
    ti_val_drop(arrowval);
    return e->nr;
}

static int cq__f_find(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_arrow_t * arrow = NULL;
    ti_val_t * iterval = ti_query_val_pop(query);
    cleri_node_t * arrow_nd;
    _Bool from_rval = iterval == query->rval;
    ti_thing_t * iter_thing = iterval
            ? (iterval->tp == TI_VAL_THING
                    ? iterval->via.thing
                    : NULL)
            : query_get_thing(query);

    if (iterval && (!ti_val_is_iterable(iterval) || iterval->tp == TI_VAL_RAW))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `find`",
                query_tp_str(query));
        return e->nr;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `find` find 1 argument but %d were given", n);
        return e->nr;
    }

    /* clear query->rval (bug keep the value)
     * from here need need to clean iterval */
    query->rval = NULL;
    if (ti_cq_scope(query, nd->children->node, e))
        goto failed;

    if (query->rval->tp != TI_VAL_ARROW)
    {
        ex_set(e, EX_BAD_DATA,
                "function `find` expects an `%s` but got type `%s` instead",
                ti_val_tp_str(TI_VAL_ARROW),
                ti_val_tp_str(query->rval->tp));
        goto failed;
    }

    arrowval = query->rval;
    query->rval = NULL;

    if (ti_scope_local_from_node(query->scope, arrowval->via.arrow, e))
        goto failed;

    arrow_nd = arrowval->via.arrow;

    if (iter_thing)
    {
        for (vec_each(iter_thing->props, ti_prop_t, p))
        {
            size_t n = 0;
            for (vec_each(query->scope->local, ti_prop_t, prop), ++n)
            {
                switch (n)
                {
                case 0:
                    /* use name as raw */
                    ti_val_weak_set(&prop->val, TI_VAL_RAW, p->name);
                    break;
                case 1:
                    ti_val_weak_copy(&prop->val, &p->val);
                    break;
                default:
                    ti_val_set_nil(&prop->val);
                }
            }

            if (ti_cq_scope(query, ti_arrow_scope_nd(arrow), e))
                goto failed;

            if (ti_val_as_bool(query->rval))
            {
                ti_val_clear(query->rval);
                if (ti_val_copy(query->rval, &p->val))
                    goto failed;
                else
                    goto done;
            }

            query_rval_weak_destroy(query);
        }
    }
    else switch (iterval->tp)
    {
    case TI_VAL_ARRAY:
    case TI_VAL_TUPLE:
        {
            int64_t idx = 0;
            for (vec_each(iterval->via.arr, ti_val_t, v), ++idx)
            {
                size_t n = 0;
                for (vec_each(query->scope->local, ti_prop_t, prop), ++n)
                {
                    switch (n)
                    {
                    case 0:
                        ti_val_weak_copy(&prop->val, v);
                        break;
                    case 1:
                        ti_val_set_int(&prop->val, idx);
                        break;
                    default:
                        ti_val_set_nil(&prop->val);
                    }
                }

                if (ti_cq_scope(query, ti_arrow_scope_nd(arrow), e))
                    goto failed;

                if (ti_val_as_bool(query->rval))
                {
                    ti_val_clear(query->rval);
                    if (ti_val_copy(query->rval, v))
                        goto failed;
                    else
                        goto done;
                }

                query_rval_destroy(query);
            }
        }
        break;
    case TI_VAL_THINGS:
        {
            int64_t idx = 0;
            for (vec_each(iterval->via.things, ti_thing_t, t), ++idx)
            {
                size_t n = 0;
                for (vec_each(query->scope->local, ti_prop_t, prop), ++n)
                {
                    switch (n)
                    {
                    case 0:
                        ti_val_weak_set(&prop->val, TI_VAL_THING, t);
                        break;
                    case 1:
                        ti_val_set_int(&prop->val, idx);
                        break;
                    default:
                        ti_val_set_nil(&prop->val);
                    }
                }

                if (ti_cq_scope(query, ti_arrow_scope_nd(arrow), e))
                    goto failed;

                if (ti_val_as_bool(query->rval))
                {
                    ti_val_clear(query->rval);
                    ti_val_set_thing(query->rval, t);
                    goto done;
                }

                query_rval_destroy(query);
            }
        }
    }

    assert (query->rval == NULL);
    query->rval = ti_val_create(TI_VAL_NIL, NULL);
    if (query->rval)
        goto done;

failed:
    if (!e->nr)  /* all not set errors are allocation errors */
        ex_set_alloc(e);

done:
    if (from_rval)
        ti_val_destroy(iterval);

    ti_val_destroy(arrowval);
    return e->nr;
}

static int cq__f_get(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    int n;
    _Bool force_as_array = true;
    ti_thing_t * thing;
    ti_val_t * value;
    vec_t * ret_array;
    ti_name_t * name;
    ti_prop_t * prop;
    vec_t * collect_attrs = NULL;
    cleri_children_t * child = nd->children;    /* first in argument list */

    if (!query_is_thing(query))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `get`",
                query_tp_str(query));
        return e->nr;
    }
    thing = query_get_thing(query);

    n = langdef_nd_n_function_params(nd);
    if (!n)
    {
        ex_set(e, EX_BAD_DATA,
                "function `get` requires at least 1 argument but 0 "
                "were given");
        return e->nr;
    }

    if (!ti_thing_with_attrs(thing))
    {
        collect_attrs = omap_get(query->collect, thing->id);
        if (!collect_attrs)
        {
            collect_attrs = vec_new(1);
            if (!collect_attrs ||
                omap_add(query->collect, thing->id, collect_attrs))
            {
                vec_destroy(collect_attrs, NULL);
                ex_set_alloc(e);
                return e->nr;
            }
        }
    }

    ret_array = vec_new(n);
    if (!ret_array)
    {
        ex_set_alloc(e);
        return e->nr;
    }

    assert (child);
    query_rval_destroy(query);

    for (n = 0; child; child = child->next->next)
    {
        ++n;
        assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);

        if (ti_cq_scope(query, child->node, e))
            goto failed;

        assert (query->rval);

        if (!ti_val_is_valid_name(query->rval))
        {
            if (ti_val_is_raw(query->rval))
            {
                ex_set(e, EX_BAD_DATA,
                        "argument %d to function `get` is not a valid name, "
                        "see "TI_DOCS"#names",
                        n);
            }
            else
            {
                ex_set(e, EX_BAD_DATA,
                        "function `get` only accepts `%s` arguments, but "
                        "argument %d is of type `%s`",
                        ti_val_tp_str(TI_VAL_RAW), n, query_tp_str(query));
            }
            goto failed;
        }

        name = ti_names_get_from_val(query->rval);
        if (!name)
            goto failed;

        ti_val_clear(query->rval);

        if (collect_attrs)
        {
            prop = ti_prop_create(name, TI_VAL_NIL, NULL);
            if (!prop || vec_push(&collect_attrs, prop))
            {
                ti_prop_destroy(prop);
                ex_set_alloc(e);
                goto failed;
            }
            ti_val_weak_set(query->rval, TI_VAL_ATTR, prop);
        }
        else
        {
            prop = ti_thing_attr_get(thing, name);
            if (!prop)
                ti_val_set_nil(query->rval);
            else
                ti_val_weak_set(query->rval, TI_VAL_ATTR, prop);
        }

        VEC_push(ret_array, query->rval);
        query->rval = NULL;

        if (!child->next)
        {
            force_as_array = false;
            break;
        }
    }

    assert (ret_array->n >= 1);

    if (n > 1 || force_as_array)
    {
        if (query_rval_clear(query))
        {
            ex_set_alloc(e);
            goto failed;
        }

        ti_val_weak_set(query->rval, TI_VAL_TUPLE, ret_array);
        goto done;
    }

    query_rval_destroy(query);
    value = vec_pop(ret_array);
    query->rval = value;

    /* bubble down to failed for vec cleanup */
    assert (!e->nr);
    assert (!ret_array->n);

failed:
    vec_destroy(ret_array, (vec_destroy_cb) ti_val_destroy);

done:
    return e->nr;
}

static int cq__f_hasprop(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_raw_t * rname;
    ti_name_t * name;
    ti_thing_t * thing;

    if (!query_is_thing(query))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `hasprop`",
                query_tp_str(query));
        return e->nr;
    }
    thing = query_get_thing(query);
    assert(thing);

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `hasprop` takes 1 argument but %d were given", n);
        return e->nr;
    }

    if (ti_cq_scope(query, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `hasprop` expects argument 1 to be of type `%s` "
                "but got `%s`",
                ti_val_tp_str(TI_VAL_RAW),
                ti_val_str(query->rval));
        return e->nr;
    }

    rname = query->rval->via.raw;
    name = ti_names_weak_get((const char *) rname->data, rname->n);

    ti_val_clear(query->rval);
    ti_val_set_bool(query->rval, name && ti_thing_get(thing, name));

    return e->nr;
}

static int cq__f_id(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    int64_t thing_id;
    ti_thing_t * thing;

    if (!query_is_thing(query))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `id`",
                query_tp_str(query));
        return e->nr;
    }
    thing = query_get_thing(query);

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `id` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    thing_id = thing->id;

    if (query_rval_clear(query))
        ex_set_alloc(e);
    else
        ti_val_set_int(query->rval, thing_id);

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

static int cq__f_isinf(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    int is_inf;

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
                "function `isinf` expects an `"TI_VAL_FLOAT_S"` "
                "but got type `%s` instead",
                ti_val_str(query->rval));
        return e->nr;
    }

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_inf);

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
        ex_set(e, EX_BAD_DATA,
                "function `isnan` expects an `"TI_VAL_FLOAT_S"` "
                "but got type `%s` instead",
                ti_val_str(query->rval));
        return e->nr;
    }

    ti_val_drop(query->rval);
    query->rval = (ti_val_t *) ti_vbool_get(is_nan);

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

    query->rval = ti_vint_create((int64_t) ti_val_iterator_n(val));
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
    vec_t * retvec;
    ti_val_t * retval = NULL;
    ti_val_t * arrowval = NULL, * iterval = query_get_val(query);
    cleri_node_t * arrow_nd;
    _Bool from_rval = iterval == query->rval;
    ti_thing_t * iter_thing = iterval
            ? (iterval->tp == TI_VAL_THING
                    ? iterval->via.thing
                    : NULL)
            : query_get_thing(query);

    if (iterval && (!ti_val_is_iterable(iterval) || iterval->tp == TI_VAL_RAW))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `map`",
                query_tp_str(query));
        return e->nr;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `map` takes 1 argument but %d were given", n);
        return e->nr;
    }

    /* clear query->rval (bug keep the value)
     * from here need need to clean iterval */
    query->rval = NULL;
    if (ti_cq_scope(query, nd->children->node, e))
        goto failed;

    if (query->rval->tp != TI_VAL_ARROW)
    {
        ex_set(e, EX_BAD_DATA,
                "function `map` expects an `%s` but got type `%s` instead",
                ti_val_tp_str(TI_VAL_ARROW),
                ti_val_tp_str(query->rval->tp));
        goto failed;
    }

    n = iter_thing ? iter_thing->props->n : iterval->via.arr->n;
    arrowval = query->rval;
    query->rval = NULL;

    if (ti_scope_local_from_node(query->scope, arrowval->via.arrow, e))
        goto failed;

    arrow_nd = arrowval->via.arrow;

    retvec = vec_new(n);
    if (!retvec)
        goto failed;

    retval = ti_val_weak_create(TI_VAL_THINGS, retvec);
    if (!retval)
    {
        vec_destroy(retvec, NULL);
        goto failed;
    }

    if (iter_thing)
    {
        for (vec_each(iter_thing->props, ti_prop_t, p))
        {
            size_t paramn = 0;
            for (vec_each(query->scope->local, ti_prop_t, prop), ++paramn)
            {
                switch (paramn)
                {
                case 0:
                    /* use name as raw */
                    ti_val_weak_set(&prop->val, TI_VAL_RAW, p->name);
                    break;
                case 1:
                    ti_val_weak_copy(&prop->val, &p->val);
                    break;
                default:
                    ti_val_set_nil(&prop->val);
                }
            }

            if (ti_cq_scope(query, ti_arrow_scope_nd(arrow), e))
                goto failed;

            if (ti_val_move_to_arr(retval, query->rval, e))
                goto failed;

            query->rval = NULL;
        }
    }
    else switch (iterval->tp)
    {
    case TI_VAL_ARRAY:
    case TI_VAL_TUPLE:
        {
            int64_t idx = 0;
            for (vec_each(iterval->via.arr, ti_val_t, v), ++idx)
            {
                size_t paramn = 0;
                for (vec_each(query->scope->local, ti_prop_t, prop), ++paramn)
                {
                    switch (paramn)
                    {
                    case 0:
                        ti_val_weak_copy(&prop->val, v);
                        break;
                    case 1:
                        ti_val_set_int(&prop->val, idx);
                        break;
                    default:
                        ti_val_set_nil(&prop->val);
                    }
                }

                if (ti_cq_scope(query, ti_arrow_scope_nd(arrow), e))
                    goto failed;

                if (ti_val_move_to_arr(retval, query->rval, e))
                    goto failed;

                query->rval = NULL;
            }
        }
        break;
    case TI_VAL_THINGS:
        {
            int64_t idx = 0;
            for (vec_each(iterval->via.things, ti_thing_t, t), ++idx)
            {
                size_t paramn = 0;
                for (vec_each(query->scope->local, ti_prop_t, prop), ++paramn)
                {
                    switch (paramn)
                    {
                    case 0:
                        ti_val_weak_set(&prop->val, TI_VAL_THING, t);
                        break;
                    case 1:
                        ti_val_set_int(&prop->val, idx);
                        break;
                    default:
                        ti_val_set_nil(&prop->val);
                    }
                }

                if (ti_cq_scope(query, ti_arrow_scope_nd(arrow), e))
                    goto failed;

                if (ti_val_move_to_arr(retval, query->rval, e))
                    goto failed;

                query->rval = NULL;
            }
        }
    }

    assert (query->rval == NULL);
    assert (retvec->n == n);
    query->rval = retval;

    goto done;

failed:
    ti_val_destroy(retval);
    if (!e->nr)  /* all not set errors are allocation errors */
        ex_set_alloc(e);

done:
    if (from_rval)
        ti_val_destroy(iterval);

    ti_val_destroy(arrowval);
    return e->nr;
}

static int cq__f_now(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query_get_thing(query) == query->target->root);

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `now` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    if (query_rval_clear(query))
        ex_set_alloc(e);
    else
        ti_val_set_float(query->rval, util_now());

    return e->nr;
}

static int cq__f_pop(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * val = query_get_val(query);
    _Bool from_rval = val == query->rval;
    void * last;

    if (!val || !ti_val_is_mutable_arr(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `pop`",
                query_tp_str(query));
        return e->nr;
    }

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `pop` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    last = vec_pop(val->via.arr);

    if (!last)
    {
        ex_set(e, EX_INDEX_ERROR, "pop from empty array");
        return e->nr;
    }

    (void) vec_shrink(&val->via.arr);

    query->rval = val->tp == TI_VAL_THINGS
            ? ti_val_create(TI_VAL_THING, last)
            : last;

    if (!query->rval)
    {
        ex_set_alloc(e);
        goto done;
    }

    if (!from_rval)
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
                val->via.arr->n,
                1,
                0))
            ex_set_alloc(e);
    }

done:
    if (from_rval)
        ti_val_destroy(val);

    return e->nr;
}

static int cq__f_push(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    int n;
    cleri_children_t * child = nd->children;    /* first in argument list */
    uint32_t current_n, new_n;

    ti_val_t * val = query_get_val(query);
    _Bool from_rval = val == query->rval;

    if (!val || !ti_val_is_mutable_arr(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `push`",
                query_tp_str(query));
        return e->nr;
    }

    n = langdef_nd_n_function_params(nd);
    if (!n)
    {
        ex_set(e, EX_BAD_DATA,
                "function `push` requires at least 1 argument but 0 "
                "were given");
        return e->nr;
    }

    if (!from_rval && ti_scope_current_val_in_use(query->scope))
    {
        ex_set(e, EX_BAD_DATA,
                "cannot use function `push` while the array is in use");
        return e->nr;
    }

    current_n = val->via.arr->n;
    new_n = current_n + n;

    if (new_n >= query->target->quota->max_array_size)
    {
        ex_set(e, EX_MAX_QUOTA,
                "maximum array size quota of %zu has been reached, "
                "see "TI_DOCS"#quotas", query->target->quota->max_array_size);
        return e->nr;
    }

    if (vec_resize(&val->via.arr, new_n))
    {
        ex_set_alloc(e);
        return e->nr;
    }

    assert (child);

    /* we have rval saved to val */
    query->rval = NULL;

    for (; child; child = child->next->next)
    {
        assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);

        if (ti_cq_scope(query, child->node, e))
            goto failed;

        if (ti_val_move_to_arr(val, query->rval, e))
            goto failed;

        query->rval = NULL;

        if (!child->next)
            break;
    }

    if (!from_rval)
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
                val,
                current_n,
                0,
                n))
            goto alloc_err;  /* we do not need to cleanup task, since the task
                                is added to `query->ev->tasks` */
    }

    assert (e->nr == 0);

    goto done;

alloc_err:
    ex_set_alloc(e);

failed:
    query_rval_destroy(query);
    while (--n)
    {
        if (val->tp == TI_VAL_THINGS)
            ti_thing_drop(vec_pop(val->via.things));
        else
            ti_val_destroy(vec_pop(val->via.array));
    }
    (void) vec_shrink(&val->via.arr);

done:
    if (from_rval)
        query->rval = val;
    return e->nr;
}

static int cq__f_rename(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    int n;
    ti_thing_t * thing;
    cleri_node_t * from_nd, * to_nd;
    ti_val_t * from_val;
    ti_raw_t * from_raw, * to_raw;
    ti_name_t * from_name, * to_name;
    ti_task_t * task;

    if (!query_is_thing(query))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `del`",
                query_tp_str(query));
        return e->nr;
    }

    thing = query_get_thing(query);
    if (!thing->id)
    {
        ex_set(e, EX_BAD_DATA,
                "function `rename` requires a thing to be assigned, "
                "`del` should therefore be used in a separate statement");
        return e->nr;
    }

    n = langdef_nd_n_function_params(nd);
    if (n != 2)
    {
        ex_set(e, EX_BAD_DATA,
                "function `rename` expects 2 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    /* TODO: maybe this is possible? */
    if (query_in_use_thing(query))
    {
        ex_set(e, EX_BAD_DATA,
                "cannot use `rename` while thing "TI_THING_ID" is in use");
        return e->nr;
    }

    from_nd = nd
            ->children->node;
    to_nd = nd
            ->children->next->next->node;

    if (ti_cq_scope(query, from_nd, e))
        return e->nr;

    from_val = query->rval;
    if (!ti_val_is_raw(from_val))
    {
        ex_set(e, EX_BAD_DATA,
                "function `rename` expects argument 1 to be of type `%s` "
                "but got `%s`",
                ti_val_tp_str(TI_VAL_RAW),
                ti_val_str(query->rval));
        return e->nr;
    }

    from_raw = from_val->via.raw;
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
        return e->nr;
    }

    query->rval = NULL;

    if (ti_cq_scope(query, to_nd, e))
        goto finish;

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
                    "function `rename` expects argument 2 to be of type `%s` "
                    "but got `%s`",
                    ti_val_tp_str(TI_VAL_RAW),
                    ti_val_str(query->rval));
        }
        goto finish;
    }


    to_raw = query->rval->via.raw;
    to_name = ti_names_get((const char *) to_raw->data, to_raw->n);
    if (!to_name)
    {
        ex_set_alloc(e);
        goto finish;
    }

    if (!ti_thing_rename(thing, from_name, to_name))
    {
        ti_name_drop(to_name);
        ex_set(e, EX_INDEX_ERROR,
                "thing "TI_THING_ID" has no property `%.*s`",
                thing->id,
                (int) from_raw->n, (const char *) from_raw->data);
        goto finish;
    }

    task = ti_task_get_task(query->ev, thing, e);
    if (!task)
        goto finish;

    if (ti_task_add_rename(task, from_raw, to_raw))
        goto alloc_err;  /* we do not need to cleanup task, since the task
                            is added to `query->ev->tasks` */

    ti_val_clear(query->rval);

    goto finish;

alloc_err:
    ex_set_alloc(e);

finish:
    ti_val_destroy(from_val);
    return e->nr;
}

static int cq__f_refs(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_thing_t * thing;

    if (!query_is_thing(query))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `refs`",
                query_tp_str(query));
        return e->nr;
    }
    thing = query_get_thing(query);

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `re` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    if (query_rval_clear(query))
        ex_set_alloc(e);
    else
        ti_val_set_int(query->rval, (int64_t) thing->ref);

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

    if (query_rval_clear(query))
        ex_set_alloc(e);
    else
        ti_val_set_nil(query->rval);

    return e->nr;
}

static int cq__f_set(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    int n;
    cleri_node_t * name_nd, * value_nd;
    ti_raw_t * rname;
    ti_task_t * task;
    ti_name_t * name;
    ti_thing_t * thing = (ti_thing_t *) ti_query_val_pop(query);

    if (thing->tp != TI_VAL_THING)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `set`",
                ti_val_str((ti_val_t *) thing));
        return e->nr;
    }

    if (!thing->id)
    {
        ex_set(e, EX_BAD_DATA,
                "function `set` requires a thing to be assigned, "
                "`set` should therefore be used in a separate statement");
        return e->nr;
    }

    n = langdef_nd_n_function_params(nd);
    if (n != 2)
    {
        ex_set(e, EX_BAD_DATA,
                "function `set` expects 2 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    name_nd = nd
            ->children->node;
    value_nd = nd
            ->children->next->next->node;

    if (ti_cq_scope(query, name_nd, e))
        return e->nr;

    if (!ti_val_is_valid_name(query->rval))
    {
        if (ti_val_is_raw(query->rval))
        {
            ex_set(e, EX_BAD_DATA,
                    "function `set` expects argument 1 to be a valid name, "
                    "see "TI_DOCS"#names");
        }
        else
        {
            ex_set(e, EX_BAD_DATA,
                    "function `set` expects argument 1 to be of type `%s` "
                    "but got `%s`",
                    ti_val_tp_str(TI_VAL_RAW),
                    ti_val_str(query->rval));
        }
        return e->nr;
    }

    rname = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_cq_scope(query, value_nd, e))
        goto finish;

    if (!ti_val_is_settable(query->rval))
    {
        ex_set(e, EX_BAD_DATA, "type `%s` is not settable",
                ti_val_str(query->rval));
        goto finish;
    }

    task = ti_task_get_task(query->ev, thing, e);
    if (!task)
        goto finish;

    name = ti_names_get((const char *) rname->data, rname->n);
    if (!name || ti_task_add_set(task, name, query->rval))
        goto alloc_err;  /* we do not need to cleanup task, since the task
                            is added to `query->ev->tasks` */

    /* TODO: we can technically already store attributes for the desired state,
     * when in away mode, the `other` attributes must be retrieved so what do
     * we win? We cannot set the `attribute` flag since that indicates as if we
     * have all attributes
     */
    if (!ti_thing_with_attrs(thing))
    {
        ti_val_drop(query->rval);
    }
    else if (ti_thing_attr_set(thing, name, query->rval))
        goto alloc_err;

    query->rval = (ti_val_t *) ti_nil_get();

    goto finish;

alloc_err:
    /* we happen to require cleanup of name when alloc_err is used */
    ti_name_drop(name);
    ex_set_alloc(e);

finish:
    ti_val_drop((ti_val_t *) rname);
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
                "cannot use function `splice` while the array is in use");
        goto done;
    }

    if (ti_cq_scope(query, child->node, e))
        goto done;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `splice` expects argument 1 to be of "
                "type `"TI_VAL_INT_S"` but got `%s`",
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
                "function `splice` expects argument 2 to be of type `%s` "
                "but got `%s`",
                ti_val_tp_str(TI_VAL_INT),
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

        if (ti_varr_append(varr, query->rval, e))
            goto failed;

        ti_decref(query->rval);
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
        ti_val_destroy(vec_pop(varr->vec));

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

    if (val->tp != TI_VAL_RAW)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `startswith`",
                query_tp_str(query));
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
                "type `"TI_VAL_RAW_S"` but got `%s`",
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
                "of type `"TI_VAL_REGEX_S"` but got `%s`",
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

    int n;
    ti_varr_t * varr = NULL;
    cleri_children_t * child = nd->children;    /* first in argument list */

    n = langdef_nd_n_function_params(nd);
    if (!n)
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
                assert (n == 1);
                ti_incref(thing);
                query->rval = (ti_val_t *) thing;
                if (ti_scope_push_thing(&query->scope, thing))
                    ex_set_alloc(e);

                return e->nr; /* only one thing, no array */
            }

            varr = ti_varr_create(n);
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
    int n = langdef_nd_n_function_params(nd);
    ex_enum errnr;
    ti_val_t * ret;

    if (n < 1)
    {
        ex_set(e, EX_BAD_DATA,
                "function `try` requires at least 1 argument but 0 "
                "were given");
        return e->nr;
    }

    errnr = ti_cq_scope(query, child->node, e);

    if (errnr > -100)
        return errnr;   /* return when successful or internal errors */

    e->nr = 0;

    if (n == 1)
        return 0;

    child = child->next->next;

    ti_val_drop(query->rval);
    query->rval = NULL;

    if (ti_cq_scope(query, child->node, e))
        return e->nr;

    if (n == 2)
        return 0;

    ret = query->rval;
    query->rval = NULL;

    for (child = child->next->next; child; child = child->next->next)
    {
        if (ti_cq_scope(query, child->node, e))
            goto failed;

        if (ti_val_convert_to_errnr(&query->rval, e))
            goto failed;

        assert (query->rval->tp == TI_VAL_INT);

        if ((ex_enum) ((ti_vint_t * ) query->rval)->int_ == errnr)
        {
            ti_val_drop(query->rval);
            query->rval = ret;
            assert (e->nr == 0);
            return 0;   /* catch the error */
        }

        if (!child->next)
            break;
    }

    e->nr = errnr;

failed:
    ti_val_destroy(ret);
    return e->nr;
}

static int cq__f_unset(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    cleri_node_t * name_nd;
    ti_task_t * task;
    ti_raw_t * rname;
    ti_name_t * name = NULL;
    ti_thing_t * thing;

    thing = (ti_thing_t *) ti_query_val_pop(query);

    if (thing->tp != TI_VAL_THING)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `unset`",
                ti_val_str((ti_val_t *) thing));
        goto done;
    }

    if (!thing->id)
    {
        ex_set(e, EX_BAD_DATA,
                "function `unset` requires a thing to be assigned, "
                "`unset` should therefore be used in a separate statement");
        goto done;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `unset` takes 1 argument but %d were given", n);
        goto done;
    }

    name_nd = nd->children->node;

    if (ti_cq_scope(query, name_nd, e))
        return e->nr;

    if (!ti_val_is_valid_name(query->rval))
    {
        if (ti_val_is_raw(query->rval))
        {
            ex_set(e, EX_BAD_DATA,
                    "function `unset` expects argument 1 to be a valid name, "
                    "see "TI_DOCS"#names");
        }
        else
        {
            ex_set(e, EX_BAD_DATA,
                    "function `unset` expects argument 1 to be "
                    "of type `"TI_VAL_RAW_S"` but got `%s`",
                    ti_val_str(query->rval));
        }
        return e->nr;
    }

    task = ti_task_get_task(query->ev, thing, e);
    if (!task)
        goto done;

    rname = (ti_raw_t *) query->rval;
    name = ti_names_get((const char *) rname->data, rname->n);

    if (!name || ti_task_add_unset(task, name))
        goto alloc_err;  /* we do not need to cleanup task, since the task
                            is added to `query->ev->tasks` */

    /* unset can run even in case this is a not managed thing */
    ti_thing_attr_unset(thing, name);

    ti_val_clear(query->rval);

    goto done;

alloc_err:
    ex_set_alloc(e);

done:
    ti_name_drop(name);
    ti_val_drop((ti_val_t *) thing);
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
    case CLERI_GID_F_GET:
        return cq__f_get(query, params, e);
    case CLERI_GID_F_HASPROP:
        return cq__f_hasprop(query, params, e);
    case CLERI_GID_F_ID:
        return cq__f_id(query, params, e);
    case CLERI_GID_F_INT:
        if (is_scope)
            return cq__f_int(query, params, e);
        break;
    case CLERI_GID_F_ISARRAY:
        if (is_scope)
            return cq__f_isarray(query, params, e);
        break;
    case CLERI_GID_F_ISINF:
        if (is_scope)
            return cq__f_isinf(query, params, e);
        break;
    case CLERI_GID_F_ISLIST:
        if (is_scope)
            return cq__f_islist(query, params, e);
        break;
    case CLERI_GID_F_ISNAN:
        if (is_scope)
            return cq__f_isnan(query, params, e);
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
    case CLERI_GID_F_RENAME:
        return cq__f_rename(query, params, e);
    case CLERI_GID_F_REFS:
        return cq__f_refs(query, params, e);
    case CLERI_GID_F_RET:
        return cq__f_ret(query, params, e);
    case CLERI_GID_F_SET:
        return cq__f_set(query, params, e);
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
    case CLERI_GID_F_UNSET:
        return cq__f_unset(query, params, e);
    case CLERI_GID_F_UPPER:
        return cq__f_upper(query, params, e);
    }

    ex_set(e, EX_INDEX_ERROR,
            "type `%s` has no function `%.*s`",
            query_tp_str(query),
            fname->len,
            fname->str);

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
            ->children->next->node;     /* int */

        assert (node->cl_obj->gid == CLERI_GID_T_INT);

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
        idx = ((ti_vint_t * ) node->data)->int_;

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

        query_rval_destroy(query);
        return cq__operations(query, nd->children->next->next->node, e);

    case CLERI_GID_OPR7_CMP_OR:
        if (    cq__operations(query, nd->children->node, e) ||
                ti_val_as_bool(query->rval))
            return e->nr;

        query_rval_destroy(query);
        return cq__operations(query, nd->children->next->next->node, e);

    default:
        assert (0);
    }

    ti_val_destroy(a_val);
    return e->nr;
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
                "property `%.*s` is undefined",
                (int) nd->len, nd->str);
        return e->nr;
    }

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
    ti_thing_drop(thing);
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
    assert (query_is_thing(query));

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

    query_rval_destroy(query);

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
    else if (ti_val_make_assignable(query->rval, e))
        goto failed;

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

    query->rval = (ti_val_t *) ti_nil_get();

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
