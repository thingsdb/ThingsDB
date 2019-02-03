/*
 * ti/cq.c
 */
#include <assert.h>
#include <langdef/langdef.h>
#include <langdef/nd.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/arrow.h>
#include <ti/cq.h>
#include <ti/names.h>
#include <ti/opr.h>
#include <ti/prop.h>
#include <ti/regex.h>
#include <util/query.h>
#include <util/strx.h>
#include <util/util.h>
#include <math.h>

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
static int cq__f_isarray(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_isinf(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_islist(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_isnan(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_len(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_lower(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_map(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_now(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_push(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_rename(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_ret(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_set(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_splice(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_startswith(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_str(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_test(ti_query_t * query, cleri_node_t * nd, ex_t * e);
static int cq__f_thing(ti_query_t * query, cleri_node_t * nd, ex_t * e);
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
    case CLERI_GID_OPERATIONS:
        /* skip the sequence , jump to the priority list */
        query_rval_destroy(query);
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
    case CLERI_GID_FUNCTION:
        if (cq__function(query, node, true, e))
            return e->nr;
        break;
    case CLERI_GID_NAME:
        if (cq__scope_name(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_PRIMITIVES:
        if (cq__primitives(query, node, e))
            return e->nr;
        break;
    case CLERI_GID_THING:
        if (cq__scope_thing(query, node, e))
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
        ti_val_clear(query->rval);
        ti_val_set_bool(query->rval, (nots & 1) ^ b);
    }
    else if (query->rval->tp == TI_VAL_THING)
        ti_val_mark_fetch(query->rval);

done:
    ti_scope_leave(&query->scope, current_scope);
    return e->nr;
}

static int cq__array(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_ARRAY);

    ti_val_t * val;
    vec_t * arr;
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

    arr = vec_new(sz);
    if (!arr)
    {
        ex_set_alloc(e);
        return e->nr;
    }

    val = ti_val_weak_create(TI_VAL_THINGS, arr);
    if (!val)
    {
        vec_destroy(arr, NULL);
        ex_set_alloc(e);
        return e->nr;
    }

    for (; child; child = child->next->next)
    {
        if (ti_cq_scope(query, child->node, e))
            goto failed;

        if (ti_val_move_to_arr(val, query->rval, e))
            goto failed;

        query->rval = NULL;

        if (!child->next)
            break;
    }

    query->rval = val;
    return 0;

failed:
    ti_val_destroy(val);
    return e->nr;
}


static int cq__arrow(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_ARROW);

    if (query_rval_clear(query))
        ex_set_alloc(e);
    else
        ti_val_set_arrow(query->rval, nd);
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

    if (!query_is_thing(query))
    {
        ex_set(e, EX_BAD_DATA, "cannot assign properties to `%s` type",
                query_tp_str(query));
        return e->nr;
    }
    thing = query_get_thing(query);

    name = ti_names_get(name_nd->str, name_nd->len);
    if (!name)
        goto alloc_err;

    if (assign_nd->len == 1)
    {
        if (thing->props->n == max_props)
        {
            ex_set(e, EX_MAX_QUOTA,
                    "maximum properties quota of %zu has been reached, "
                    "see "TI_DOCS"#quotas", max_props);
            return e->nr;
        }
    }
    else
    {
        assert (assign_nd->len == 2);
        assert (assign_nd->str[1] == '=');
        left_val = ti_thing_get(thing, name);
        if (!left_val)
        {
            ex_set(e, EX_INDEX_ERROR,
                    "`%.*s` is undefined", (int) name->n, name->str);
            return e->nr;
        }
    }

    /* should also work in chain because then scope->local must be NULL */
    if (    ti_scope_has_local_name(query->scope, name) ||
            ti_scope_in_use_name(query->scope, thing, name))
    {
        ex_set(e, EX_BAD_DATA,
            "cannot assign a new value to `%.*s` while the property is in use",
            (int) name->n, name->str);
        return e->nr;
    }

    if (ti_cq_scope(query, scope_nd, e))
        goto done;

    assert (query->rval);

    if (assign_nd->len == 2)
    {
        assert (left_val);
        if (ti_opr_a_to_b(left_val, assign_nd, query->rval, e))
            goto done;
    }
    else if (ti_val_check_assignable(query->rval, false, e))
        goto done;

    if (thing->id)
    {
        task = ti_task_get_task(query->ev, thing, e);

        if (!task)
            goto done;

        if (ti_task_add_assign(task, name, query->rval))
            goto alloc_err;  /* we do not need to cleanup task, since the task
                                is added to `query->ev->tasks` */
    }
    /*
     * In case we really want the assigned value, we can copy the value
     * although this is not preferred since we then have to make a copy
     */
    if (ti_thing_weak_setv(thing, name, query->rval))
    {
        if (task)
            free(vec_pop(task->jobs));
        goto alloc_err;
    }

    ti_val_set_nil(query->rval);

    goto done;

alloc_err:
    ex_set_alloc(e);

done:
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
    ti_thing_t * thing;

    if (!query_is_thing(query))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no property `%.*s`",
                query_tp_str(query),
                (int) nd->len, nd->str);
        return e->nr;
    }

    thing = query_get_thing(query);

    name = ti_names_weak_get(nd->str, nd->len);
    val = name ? ti_thing_get(thing, name) : NULL;

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

    query_rval_destroy(query);

    return e->nr;
}

static int cq__f_blob(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query_get_thing(query) == query->target->root);

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
                "function `blob` expects argument 1 to be of type `%s` "
                "but got `%s`", ti_val_tp_str(TI_VAL_INT), query_tp_str(query));
        return e->nr;
    }

    idx = query->rval->via.int_;

    if (idx < 0)
        idx += n_blobs;

    if (idx < 0 || idx >= n_blobs)
    {
        ex_set(e, EX_INDEX_ERROR, "blob index out of range");
        return e->nr;
    }

    /* clearing the previous value is not required since this was an integer */
    if (ti_val_set(query->rval, TI_VAL_RAW, vec_get(query->blobs, idx)))
        ex_set_alloc(e);

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
    ti_thing_t * thing;
    ti_raw_t * rname;

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
                "function `del` requires a thing to be assigned, "
                "`del` should therefore be used in a separate statement");
        return e->nr;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `del` takes 1 argument but %d were given", n);
        return e->nr;
    }

    if (query_in_use_thing(query))
    {
        ex_set(e, EX_BAD_DATA,
                "cannot use `del` while thing "TI_THING_ID" is in use");
        return e->nr;
    }

    name_nd = nd
            ->children->node;

    if (ti_cq_scope(query, name_nd, e))
        return e->nr;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `del` expects argument 1 to be of type `%s` "
                "but got `%s`",
                ti_val_tp_str(TI_VAL_RAW),
                ti_val_str(query->rval));
        return e->nr;
    }

    rname = query->rval->via.raw;
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
        return e->nr;
    }

    task = ti_task_get_task(query->ev, thing, e);
    if (!task)
        return e->nr;

    if (ti_task_add_del(task, rname))
    {
        ex_set_alloc(e);
        return e->nr;
    }

    ti_val_clear(query->rval);

    return e->nr;
}

static int cq__f_endswith(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * val = query_get_val(query);
    _Bool from_rval = val == query->rval;
    _Bool endswith;

    if (!val || !ti_val_is_raw(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `endswith`",
                query_tp_str(query));
        return e->nr;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `endswith` takes 1 argument but %d were given", n);
        return e->nr;
    }

    query->rval = NULL;

    if (ti_cq_scope(query, nd->children->node, e))
        goto done;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `endswith` expects argument 1 to be of type `%s` "
                "but got `%s`",
                ti_val_tp_str(TI_VAL_RAW),
                ti_val_str(query->rval));
        goto done;
    }

    endswith = ti_val_endswith(val, query->rval);

    ti_val_clear(query->rval);
    ti_val_set_bool(query->rval, endswith);

done:
    if (from_rval)
        ti_val_destroy(val);
    return e->nr;
}

static int cq__f_filter(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

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
                "type `%s` has no function `filter`",
                query_tp_str(query));
        return e->nr;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `filter` takes 1 argument but %d were given", n);
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
                "function `filter` expects an `%s` but got type `%s` instead",
                ti_val_tp_str(TI_VAL_ARROW),
                ti_val_tp_str(query->rval->tp));
        goto failed;
    }

    arrowval = query->rval;
    query->rval = NULL;

    if (ti_scope_local_from_arrow(query->scope, arrowval->via.arrow, e))
        goto failed;

    arrow_nd = arrowval->via.arrow;

    if (iter_thing)
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

        retval = ti_val_weak_create(TI_VAL_THING, thing);
        if (!retval)
        {
            ti_thing_drop(thing);
            goto failed;
        }

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

            if (ti_cq_scope(query, ti_arrow_scope_nd(arrow_nd), e))
                goto failed;

            if (ti_val_as_bool(query->rval))
            {
                ti_incref(p->name);
                if (ti_thing_setv(thing, p->name, &p->val))
                    goto failed;
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
            vec_t * retvec = vec_new(iterval->via.arr->n);
            if (!retvec)
                goto failed;

            retval = ti_val_weak_create(TI_VAL_ARRAY, retvec);
            if (!retval)
            {
                vec_destroy(retvec, NULL);
                goto failed;
            }

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

                if (ti_cq_scope(query, ti_arrow_scope_nd(arrow_nd), e))
                    goto failed;

                if (ti_val_as_bool(query->rval))
                {
                    ti_val_t * dup = ti_val_dup(v);
                    if (!dup)
                        goto failed;
                    VEC_push(retvec, dup);
                }
                query_rval_destroy(query);
            }
            (void) vec_shrink(&retval->via.array);
        }
        break;
    case TI_VAL_THINGS:
        {
            int64_t idx = 0;
            vec_t * retvec = vec_new(iterval->via.arr->n);
            if (!retvec)
                goto failed;

            retval = ti_val_weak_create(TI_VAL_THINGS, retvec);
            if (!retval)
            {
                vec_destroy(retvec, NULL);
                goto failed;
            }
            retval->flags |= TI_VAL_FLAG_FETCH;

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

                if (ti_cq_scope(query, ti_arrow_scope_nd(arrow_nd), e))
                    goto failed;

                if (ti_val_as_bool(query->rval))
                {
                    ti_incref(t);
                    VEC_push(retvec, t);
                }
                query_rval_destroy(query);
            }
            (void) vec_shrink(&retval->via.array);
        }
    }

    assert (query->rval == NULL);
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

static int cq__f_find(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

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

    if (ti_scope_local_from_arrow(query->scope, arrowval->via.arrow, e))
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

            if (ti_cq_scope(query, ti_arrow_scope_nd(arrow_nd), e))
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

                if (ti_cq_scope(query, ti_arrow_scope_nd(arrow_nd), e))
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

                if (ti_cq_scope(query, ti_arrow_scope_nd(arrow_nd), e))
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

static int cq__f_isarray(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query_get_thing(query) == query->target->root);

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

    ti_val_clear(query->rval);
    ti_val_set_bool(query->rval, is_array);

    return e->nr;
}

static int cq__f_isinf(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query_get_thing(query) == query->target->root);

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
        is_inf = isinf(query->rval->via.float_);
        break;
    default:
        ex_set(e, EX_BAD_DATA,
                "function `isinf` expects an `%s` but got type `%s` instead",
                ti_val_tp_str(TI_VAL_FLOAT),
                ti_val_tp_str(query->rval->tp));
        return e->nr;
    }

    ti_val_clear(query->rval);
    ti_val_set_bool(query->rval, is_inf);

    return e->nr;
}

static int cq__f_islist(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query_get_thing(query) == query->target->root);

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

    ti_val_clear(query->rval);
    ti_val_set_bool(query->rval, is_list);

    return e->nr;
}


static int cq__f_isnan(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query_get_thing(query) == query->target->root);

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
        is_nan = isnan(query->rval->via.float_);
        break;
    default:
        ex_set(e, EX_BAD_DATA,
                "function `isnan` expects an `%s` but got type `%s` instead",
                ti_val_tp_str(TI_VAL_FLOAT),
                ti_val_tp_str(query->rval->tp));
        return e->nr;
    }

    ti_val_clear(query->rval);
    ti_val_set_bool(query->rval, is_nan);

    return e->nr;
}


static int cq__f_len(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * val = query_get_val(query);

    if (!val || !ti_val_is_iterable(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `len`",
                query_tp_str(query));
        return e->nr;
    }

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `len` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    if (query_rval_clear(query))
    {
        ex_set_alloc(e);
        return e->nr;
    }

    switch (val->tp)
    {
    case TI_VAL_RAW:
        ti_val_set_int(query->rval, val->via.raw->n);
        break;
    case TI_VAL_ARRAY:
    case TI_VAL_TUPLE:
    case TI_VAL_THINGS:
        ti_val_set_int(query->rval, val->via.arr->n);
        break;
    case TI_VAL_THING:
        ti_val_set_int(query->rval, val->via.thing->props->n);
        break;
    }

    assert (query->rval->tp == TI_VAL_INT);

    return e->nr;
}

static int cq__f_lower(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * val = query_get_val(query);
    ti_raw_t * lower;

    if (!val || !ti_val_is_raw(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `lower`",
                query_tp_str(query));
        return e->nr;
    }

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `lower` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    lower = ti_raw_lower(val->via.raw);
    if (!lower || query_rval_clear(query))
        ex_set_alloc(e);
    else
        ti_val_weak_set(query->rval, TI_VAL_RAW, lower);

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

    if (ti_scope_local_from_arrow(query->scope, arrowval->via.arrow, e))
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

            if (ti_cq_scope(query, ti_arrow_scope_nd(arrow_nd), e))
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

                if (ti_cq_scope(query, ti_arrow_scope_nd(arrow_nd), e))
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
            retval->flags |= TI_VAL_FLAG_FETCH;
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

                if (ti_cq_scope(query, ti_arrow_scope_nd(arrow_nd), e))
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

        if (ti_task_add_push(task, query->scope->name, val, n))
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

    cleri_node_t * name_nd, * value_nd;
    ti_val_t * name_val;
    ti_task_t * task;
    ti_name_t * name;

    int n;
    ti_thing_t * thing;

    if (!query_is_thing(query))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `set`",
                query_tp_str(query));
        return e->nr;
    }

    thing = query_get_thing(query);
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

    name_val = query->rval;
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

    name = ti_names_get_from_val(name_val);

    if (!name || ti_task_add_set(task, name, query->rval))
        goto alloc_err;  /* we do not need to cleanup task, since the task
                            is added to `query->ev->tasks` */

    /* TODO: we can technically already store attributes for the desired state,
     * when in away mode, the `other` attributes must be retrieved so what do
     * we win? We cannot set the `attribute` flag since that indicates as if we
     * have all attributes
     */
    if (ti_thing_with_attrs(thing))
    {
        if (ti_thing_attr_weak_setv(thing, name, query->rval))
            goto alloc_err;
        ti_val_set_nil(query->rval);
    }
    else
        ti_val_clear(query->rval);

    goto finish;

alloc_err:
    /* we happen to require cleanup of name when alloc_err is used */
    ti_name_drop(name);
    ex_set_alloc(e);

finish:
    ti_val_destroy(name_val);

    return e->nr;
}

static int cq__f_splice(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    int32_t n, x, l;
    cleri_children_t * child = nd->children;    /* first in argument list */
    uint32_t current_n, new_n;
    int64_t i, c;
    vec_t * vec, * retv;

    ti_val_t * val = query_get_val(query);
    _Bool from_rval = val == query->rval;

    if (!val || !ti_val_is_mutable_arr(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `splice`",
                query_tp_str(query));
        return e->nr;
    }

    n = langdef_nd_n_function_params(nd);
    if (n < 2)
    {
        ex_set(e, EX_BAD_DATA,
                "function `splice` requires at least 2 arguments "
                "but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    if (!from_rval && ti_scope_current_val_in_use(query->scope))
    {
        ex_set(e, EX_BAD_DATA,
                "cannot use function `splice` while the array is in use");
        return e->nr;
    }

    query->rval = NULL;

    if (ti_cq_scope(query, child->node, e))
        goto finish;

    if (query->rval->tp != TI_VAL_INT)
    {
        ex_set(e, EX_BAD_DATA,
                "function `splice` expects argument 1 to be of type `%s` "
                "but got `%s`",
                ti_val_tp_str(TI_VAL_INT),
                ti_val_str(query->rval));
        goto finish;
    }

    i = query->rval->via.int_;
    child = child->next->next;
    if (ti_cq_scope(query, child->node, e))
        goto finish;

    if (query->rval->tp != TI_VAL_INT)
    {
        ex_set(e, EX_BAD_DATA,
                "function `splice` expects argument 2 to be of type `%s` "
                "but got `%s`",
                ti_val_tp_str(TI_VAL_INT),
                ti_val_str(query->rval));
        goto finish;
    }

    c = query->rval->via.int_;
    current_n = val->via.arr->n;

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
        goto finish;
    }

    if (new_n > current_n && vec_resize(&val->via.arr, new_n))
    {
        ex_set_alloc(e);
        goto finish;
    }

    retv = vec_new(c);
    if (!retv)
    {
        ex_set_alloc(e);
        goto finish;
    }

    vec = val->via.arr;

    for (x = i, l = i + c; x < l; ++x)
        VEC_push(retv, vec_get(vec, x));

    memmove(
        vec->data + i + n,
        vec->data + i + c,
        (current_n - i - c) * sizeof(void*));

    vec->n = i;

    for (x = 0; x < n; ++x)
    {
        child = child->next->next;
        assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);

        if (ti_cq_scope(query, child->node, e))
            goto failed;

        if (ti_val_move_to_arr(val, query->rval, e))
            goto failed;

        query->rval = NULL;
    }

    if (!from_rval)
    {
        assert (query->scope->thing);
        assert (query->scope->name);

        ti_task_t * task;

        task = ti_task_get_task(query->ev, query->scope->thing, e);
        if (!task)
            goto failed;

        if (ti_task_add_splice(
                task,
                query->scope->name,
                val,
                i,
                c,
                n))
            goto alloc_err;  /* we do not need to cleanup task, since the task
                                is added to `query->ev->tasks` */
    }

    assert (e->nr == 0);

    /* required since query->rval may not be NULL */
    ti_val_destroy(query->rval);

    query->rval = ti_val_weak_create(val->tp, retv);
    if (query->rval)
    {
        val->via.arr->n = new_n;
        if (new_n < current_n)
            (void) vec_shrink(&val->via.arr);
        goto finish;
    }

alloc_err:
    ex_set_alloc(e);

failed:
    vec = val->via.arr;
    while (x--)
        ti_val_destroy(vec_pop(vec));

    memmove(
        vec->data + i + n,
        vec->data + i + c,
        (current_n - i - c) * sizeof(void*));

    for (x = 0; x < c; ++x)
        VEC_push(vec, vec_get(retv, x));

    vec_destroy(retv, NULL);
    vec->n = current_n;

finish:
    if (from_rval)
        ti_val_destroy(val);

    return e->nr;
}

static int cq__f_startswith(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * val = query_get_val(query);
    _Bool from_rval = val == query->rval;
    _Bool startswith;

    if (!val || !ti_val_is_raw(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `startswith`",
                query_tp_str(query));
        return e->nr;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `startswith` takes 1 argument but %d were given", n);
        return e->nr;
    }

    query->rval = NULL;

    if (ti_cq_scope(query, nd->children->node, e))
        goto done;

    if (!ti_val_is_raw(query->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `startswith` expects argument 1 to be of type `%s` "
                "but got `%s`",
                ti_val_tp_str(TI_VAL_RAW),
                ti_val_str(query->rval));
        goto done;
    }

    startswith = ti_val_startswith(val, query->rval);

    ti_val_clear(query->rval);
    ti_val_set_bool(query->rval, startswith);

done:
    if (from_rval)
        ti_val_destroy(val);
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

    assert (query->rval == NULL);

    if (ti_cq_scope(query, nd->children->node, e))
        return e->nr;

    if (ti_val_convert_to_str(query->rval))
    {
        ex_set_alloc(e);
        return e->nr;
    }

    return 0;
}

static int cq__f_test(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * val = query_get_val(query);
    _Bool from_rval = val == query->rval;
    _Bool has_match;

    if (!val || !ti_val_is_raw(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `test`",
                query_tp_str(query));
        return e->nr;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `test` takes 1 argument but %d were given", n);
        return e->nr;
    }

    query->rval = NULL;

    if (ti_cq_scope(query, nd->children->node, e))
        goto done;

    if (query->rval->tp != TI_VAL_REGEX)
    {
        ex_set(e, EX_BAD_DATA,
                "function `test` expects argument 1 to be of type `%s` "
                "but got `%s`",
                ti_val_tp_str(TI_VAL_REGEX),
                ti_val_str(query->rval));
        goto done;
    }

    has_match = ti_regex_test(query->rval->via.regex, val->via.raw);

    ti_val_clear(query->rval);
    ti_val_set_bool(query->rval, has_match);

done:
    if (from_rval)
        ti_val_destroy(val);
    return e->nr;
}

static int cq__f_thing(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (query_get_thing(query) == query->target->root);

    int n;
    _Bool force_as_array = true;
    vec_t * things;
    ti_thing_t * thing;
    cleri_children_t * child = nd->children;    /* first in argument list */

    n = langdef_nd_n_function_params(nd);
    if (!n)
    {
        ex_set(e, EX_BAD_DATA,
                "function `thing` requires at least 1 argument but 0 "
                "were given");
        return e->nr;
    }

    things = vec_new(n);
    if (!things)
    {
        ex_set_alloc(e);
        return e->nr;
    }

    assert (child);

    for (n = 0; child; child = child->next->next)
    {
        ++n;
        assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);

        query_rval_destroy(query);
        if (ti_cq_scope(query, child->node, e))
            goto failed;

        assert (query->rval);

        if (query->rval->tp != TI_VAL_INT)
        {
            ex_set(e, EX_BAD_DATA,
                    "function `thing` only accepts `int` arguments, but "
                    "argument %d is of type `%s`", n, query_tp_str(query));
            goto failed;
        }

        thing = ti_collection_thing_by_id(query->target, query->rval->via.int_);
        if (!thing)
        {
            ex_set(e, EX_INDEX_ERROR,
                    "collection `%.*s` has no `thing` with id `%"PRId64"`",
                    (int) query->target->name->n,
                    (char *) query->target->name->data,
                    query->rval->via.int_);
            goto failed;
        }

        ti_incref(thing);
        VEC_push(things, thing);

        if (!child->next)
        {
            force_as_array = false;
            break;
        }
    }

    assert (things->n >= 1);

    if (n > 1 || force_as_array)
    {
        if (query_rval_clear(query))
        {
            ex_set_alloc(e);
            goto failed;
        }

        ti_val_weak_set(query->rval, TI_VAL_THINGS, things);
        goto done;
    }

    query_rval_destroy(query);
    thing = vec_pop(things);

    assert (thing->ref > 1);
    ti_decref(thing);

    if (ti_scope_push_thing(&query->scope, thing))
    {
        ex_set_alloc(e);
        goto failed;
    }

    /* bubble down to failed for vec cleanup */
    assert (!e->nr);
    assert (!things->n);

failed:
    vec_destroy(things, (vec_destroy_cb) ti_thing_drop);

done:
    return e->nr;
}

static int cq__f_unset(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (query->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    cleri_node_t * name_nd;
    ti_task_t * task;
    ti_name_t * name = NULL;
    ti_thing_t * thing;

    if (!query_is_thing(query))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `unset`",
                query_tp_str(query));
        return e->nr;
    }

    thing = query_get_thing(query);
    if (!thing->id)
    {
        ex_set(e, EX_BAD_DATA,
                "function `unset` requires a thing to be assigned, "
                "`unset` should therefore be used in a separate statement");
        return e->nr;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `unset` takes 1 argument but %d were given", n);
        return e->nr;
    }

    name_nd = nd
            ->children->node;

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
                    "function `unset` expects argument 1 to be of type `%s` "
                    "but got `%s`",
                    ti_val_tp_str(TI_VAL_RAW),
                    ti_val_str(query->rval));
        }
        return e->nr;
    }

    task = ti_task_get_task(query->ev, thing, e);
    if (!task)
        goto finish;

    name = ti_names_get_from_val(query->rval);

    if (!name || ti_task_add_unset(task, name))
        goto alloc_err;  /* we do not need to cleanup task, since the task
                            is added to `query->ev->tasks` */

    /* unset can run even in case this is a not managed thing */
    ti_thing_attr_unset(thing, name);

    ti_val_clear(query->rval);

    goto finish;

alloc_err:
    ex_set_alloc(e);

finish:
    ti_name_drop(name);
    return e->nr;
}

static int cq__f_upper(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * val = query_get_val(query);
    ti_raw_t * upper;

    if (!val || !ti_val_is_raw(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `upper`",
                query_tp_str(query));
        return e->nr;
    }

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `upper` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    upper = ti_raw_upper(val->via.raw);
    if (!upper || query_rval_clear(query))
        ex_set_alloc(e);
    else
        ti_val_weak_set(query->rval, TI_VAL_RAW, upper);

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
    case CLERI_GID_F_PUSH:
        return cq__f_push(query, params, e);
    case CLERI_GID_F_RENAME:
        return cq__f_rename(query, params, e);
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
    case CLERI_GID_F_TEST:
        return cq__f_test(query, params, e);
    case CLERI_GID_F_THING:
        if (is_scope)
            return cq__f_thing(query, params, e);
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
    ti_val_t * val = query_get_val(query);
    _Bool from_rval = val == query->rval;
    int64_t idx;
    ssize_t n;
    if (!val)
    {
        /* query is of thing type */
        ex_set(e, EX_BAD_DATA, "type `%s` is not indexable",
                query_tp_str(query));
        return e->nr;
    }

    /* multiple indexes are possible, for example x[0][0] */
    for (child = nd->children; child; child = child->next)
    {
        node = child->node              /* sequence  [ int ]  */
            ->children->next->node;     /* int */

        assert (node->cl_obj->gid == CLERI_GID_T_INT);

        #if TI_USE_VOID_POINTER
        idx = (intptr_t) ((intptr_t *) node->data);
        #else
        idx = strx_to_int64(node->str);
        #endif

        switch (val->tp)
        {
        case TI_VAL_RAW:
            n = val->via.raw->n;
            break;
        case TI_VAL_TUPLE:
        case TI_VAL_ARRAY:
        case TI_VAL_THINGS:
            n = val->via.arr->n;
            break;
        default:
            ex_set(e, EX_BAD_DATA, "type `%s` is not indexable",
                    ti_val_str(val));
            return e->nr;
        }

        if (idx < 0)
            idx += n;

        if (idx < 0 || idx >= n)
        {
            ex_set(e, EX_INDEX_ERROR, "index out of range");
            return e->nr;
        }

        switch (val->tp)
        {
        case TI_VAL_RAW:
            {
                int64_t c = val->via.raw->data[idx];
                if (query_rval_clear(query))
                {
                    ex_set_alloc(e);
                    return e->nr;
                }
                ti_val_set_int(query->rval, c);
            }
            break;
        case TI_VAL_ARRAY:
        case TI_VAL_TUPLE:
            {
                ti_val_t * v = vec_get(val->via.array, idx);
                if (from_rval)
                {
                    ti_val_enum tp = v->tp;
                    void * data = v->via.nil;
                    v->tp = TI_VAL_NIL;
                    /* v will now potentially be destroyed,
                     * but data and type are saved */
                    (void) query_rval_clear(query);
                    ti_val_weak_set(query->rval, tp, data);
                }
                else
                {
                    assert (query->scope->name);
                    query->scope->val = v;
                }
            }
            break;
        case TI_VAL_THINGS:
            {
                ti_thing_t * thing = vec_get(val->via.things, idx);

                if (from_rval)
                {
                    ti_incref(thing);
                    (void) query_rval_clear(query);
                    ti_val_weak_set(query->rval, TI_VAL_THING, thing);
                }

                if (ti_scope_push_thing(&query->scope, thing))
                {
                    ex_set_alloc(e);
                    return e->nr;
                }
            }
            break;
        default:
            assert (0);
            return -1;
        }
        val = query_get_val(query);
        from_rval = val == query->rval;
    }
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
        (void) ti_opr_a_to_b(a_val, nd->children->next->node, query->rval, e);
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

    if (query_rval_clear(query))
    {
        ex_set_alloc(e);
        return e->nr;
    }

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_T_FALSE:
        ti_val_set_bool(query->rval, false);
        break;
    case CLERI_GID_T_FLOAT:
        #if TI_USE_VOID_POINTER
        {
            assert (sizeof(double) == sizeof(void *));
            double d;
            memcpy(&d, &node->data, sizeof(double));
            ti_val_set_float(query->rval, d);
        }
        #else
        ti_val_set_float(query->rval, strx_to_double(node->str));
        #endif
        break;
    case CLERI_GID_T_INT:
        #if TI_USE_VOID_POINTER
        ti_val_set_int(query->rval, (intptr_t) ((intptr_t *) node->data));
        #else
        ti_val_set_int(query->rval, strx_to_int64(node->str));
        if (errno == ERANGE)
            ex_set(e, EX_OVERFLOW, "integer overflow");
        #endif
        break;
    case CLERI_GID_T_NIL:
        ti_val_set_nil(query->rval);
        break;
    case CLERI_GID_T_REGEX:
        if (!node->data)
        {
            node->data = ti_regex_from_strn(node->str, node->len, e);
            if (!node->data)
                return e->nr;
            assert (vec_space(query->nd_cache));
            VEC_push(query->nd_cache, node);
        }
        (void) ti_val_set(query->rval, TI_VAL_REGEX, node->data);
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
            VEC_push(query->nd_cache, node);
        }
        (void) ti_val_set(query->rval, TI_VAL_RAW, node->data);
        break;
    case CLERI_GID_T_TRUE:
        ti_val_set_bool(query->rval, true);
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
    assert (query_is_thing(query));

    ti_name_t * name;
    ti_val_t * val;

    name = ti_names_weak_get(nd->str, nd->len);
    val = name ? ti_scope_find_local_val(query->scope, name) : NULL;

    if (!val && name)
        val = ti_thing_get(query->target->root, name);

    if (!val)
    {
        ex_set(e, EX_INDEX_ERROR,
                "property `%.*s` is undefined",
                (int) nd->len, nd->str);
        return e->nr;
    }

    if (ti_scope_push_name(&query->scope, name, val))
        ex_set_alloc(e);

    query_rval_destroy(query);

    return e->nr;
}

static int cq__scope_thing(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    /*
     * Sequence('{', List(Sequence(name, ':', scope)), '}')
     */
    assert (e->nr == 0);
    assert (query_is_thing(query));

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

    query_rval_destroy(query);

    thing = ti_thing_create(0, query->target->things);
    if (!thing)
        goto alloc_err;

    child = nd                                  /* sequence */
            ->children->next->node              /* list */
            ->children;                         /* list items */

    for (; child; child = child->next->next)
    {
        if (thing->props->n == max_props)
        {
            ex_set(e, EX_MAX_QUOTA,
                    "maximum properties quota of %zu has been reached, "
                    "see "TI_DOCS"#quotas", max_props);
            goto err;
        }
        cleri_node_t * name_nd = child->node        /* sequence */
                ->children->node;                   /* name */

        cleri_node_t * scope = child->node          /* sequence */
                ->children->next->next->node;       /* scope */

        ti_name_t * name = ti_names_get(name_nd->str, name_nd->len);
        if (!name)
            goto alloc_err;

        if (ti_cq_scope(query, scope, e))
            goto err;

        assert (query->rval);

        ti_thing_weak_setv(thing, name, query->rval);
        query_rval_weak_destroy(query);

        if (!child->next)
            break;
    }

    if (ti_scope_push_thing(&query->scope, thing))
        goto alloc_err;

    if (query_rval_clear(query))
        goto alloc_err;

    ti_val_weak_set(query->rval, TI_VAL_THING, thing);

    goto done;

alloc_err:
    ex_set_alloc(e);
err:
    ti_thing_drop(thing);
done:
    return e->nr;
}








