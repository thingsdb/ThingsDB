/*
 * ti/res.c
 */
#include <assert.h>
#include <ti/res.h>
#include <ti/prop.h>
#include <ti/arrow.h>
#include <ti/names.h>
#include <ti/opr.h>
#include <ti.h>
#include <langdef/langdef.h>
#include <langdef/nd.h>
#include <stdlib.h>
#include <util/strx.h>
#include <util/res.h>

static int res__arrow(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__assignment(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__chain(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__chain_name(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__f_blob(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__f_endswith(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__f_filter(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__f_get(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__f_id(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__f_lower(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__f_thing(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__f_push(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__f_ret(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__f_set(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__f_startswith(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__f_unset(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__f_upper(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__function(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__index(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__operations(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__primitives(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__scope_name(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__scope_thing(ti_res_t * res, cleri_node_t * nd, ex_t * e);

ti_res_t * ti_res_create(ti_db_t * db, vec_t * blobs)
{
    assert (db && db->root);

    ti_res_t * res = malloc(sizeof(ti_res_t));
    if (!res)
        return NULL;

    res->db = db;  /* a borrowed reference since the query has one */
    res->collect = omap_create();
    res->ev = NULL;
    res->scope = NULL;
    res->rval = NULL;
    res->blobs = blobs;  /* may be NULL */
    if (!res->collect)
    {
        ti_res_destroy(res);
        return NULL;
    }
    return res;
}

void ti_res_destroy(ti_res_t * res)
{
    omap_destroy(res->collect, (omap_destroy_cb) res_destroy_collect_cb);
    ti_scope_leave(&res->scope, NULL);
    ti_val_destroy(res->rval);
    free(res);
}

int ti_res_scope(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_SCOPE);

    int nots = 0;
    cleri_node_t * node;
    cleri_children_t * nchild, * child = nd           /* sequence */
                    ->children;             /* first child, not */
    ti_scope_t * current_scope, * scope;

    for (nchild = child->node->children; nchild; nchild = nchild->next)
        ++nots;

    child = child->next;
    node = child->node                      /* choice */
            ->children->node;               /* primitives, function,
                                               assignment, name, thing,
                                               array, compare, arrow */
    current_scope = res->scope;
    scope = ti_scope_enter(current_scope, res->db->root);
    if (!scope)
    {
        ex_set_alloc(e);
        return e->nr;
    }
    res->scope = scope;


    switch (node->cl_obj->gid)
    {
    case CLERI_GID_ARRAY:
        break;
    case CLERI_GID_ARROW:
        if (res__arrow(res, node, e))
            return e->nr;
        break;
    case CLERI_GID_ASSIGNMENT:
        if (res__assignment(res, node, e))
            return e->nr;
        break;
    case CLERI_GID_OPERATIONS:
        /* skip the sequence , jump to the priority list */
        if (res__operations(res, node->children->next->node, e))
            return e->nr;
        break;
    case CLERI_GID_FUNCTION:
        if (res__function(res, node, e))
            return e->nr;
        break;
    case CLERI_GID_NAME:
        if (res__scope_name(res, node, e))
            return e->nr;
        break;
    case CLERI_GID_PRIMITIVES:
        if (res__primitives(res, node, e))
            return e->nr;
        break;
    case CLERI_GID_THING:
        if (res__scope_thing(res, node, e))
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

    if (node->children && res__index(res, node, e))
        return e->nr;

    child = child->next;
    if (!child)
        goto finish;  /* TODO:  mark_fetch = true */

    node = child->node              /* optional */
            ->children->node;       /* chain */

    assert (node->cl_obj->gid == CLERI_GID_CHAIN);

    if (res__chain(res, node, e))
        goto done;

finish:

    if (!res->rval)
    {
        res->rval = ti_scope_global_to_val(res->scope);
        if (!res->rval)
        {
            ex_set_alloc(e);
            goto done;
        }
    }

    if (nots)
    {
        _Bool b = ti_val_as_bool(res->rval);
        ti_val_clear(res->rval);
        ti_val_set_bool(res->rval, (nots & 1) ^ b);
    }
    else ti_val_mark_fetch(res->rval);  /* TODO:  if (mark_fetch) */

done:
    ti_scope_leave(&res->scope, current_scope);
    return e->nr;
}

static int res__arrow(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_ARROW);

    if (res_rval_clear(res))
        ex_set_alloc(e);
    else
        ti_val_set_arrow(res->rval, nd);
    return e->nr;
}

static int res__assignment(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_ASSIGNMENT);
    assert (res->ev);

    ti_thing_t * thing;
    ti_name_t * name;
    ti_task_t * task;
    ti_val_t * left_val = NULL;     /* assign to prevent warning */
    size_t max_props = res->db->quota->max_props;
    cleri_node_t * name_nd = nd                 /* sequence */
            ->children->node;                   /* name */
    cleri_node_t * assign_nd = nd               /* sequence */
            ->children->next->node;             /* assign tokens */
    cleri_node_t * scope_nd = nd                /* sequence */
            ->children->next->next->node;       /* scope */

    if (!res_is_thing(res))
    {
        ex_set(e, EX_BAD_DATA, "cannot assign properties to `%s` type",
                res_tp_str(res));
        return e->nr;
    }
    thing = res_get_thing(res);

    name = ti_names_get(name_nd->str, name_nd->len);
    if (!name)
        goto alloc_err;

    if (assign_nd->len == 1)
    {
        if (thing->props->n == max_props)
        {
            ex_set(e, EX_MAX_QUOTA,
                    "maximum properties quota of %u has been reached, "
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
    if (    ti_scope_has_local_name(res->scope, name) ||
            ti_scope_in_use_name(res->scope, thing, name))
    {
        ex_set(e, EX_BAD_DATA,
            "cannot assign a new value to `%.*s` while the property is in use",
            (int) name->n, name->str);
        return e->nr;
    }

    if (ti_res_scope(res, scope_nd, e))
        goto done;

    assert (res->rval);

    if (assign_nd->len == 2)
    {
        assert (left_val);
        if (ti_opr_a_to_b(left_val, assign_nd, res->rval, e))
            goto done;
    }
    else if (res_assign_val(res, false, e))
        goto done;

    if (thing->id)
    {
        task = res_get_task(res->ev, thing, e);

        if (!task)
            goto done;

        if (ti_task_add_assign(task, name, res->rval))
            goto alloc_err;  /* we do not need to cleanup task, since the task
                                is added to `res->ev->tasks` */
    }
    /*
     * In case we really want the assigned value, we can copy the value
     * although this is not preferred since we then have to make a copy
     */
    if (ti_thing_weak_setv(thing, name, res->rval))
    {
        free(vec_pop(task->jobs));
        goto alloc_err;
    }

    ti_val_set_nil(res->rval);

    goto done;

alloc_err:
    ex_set_alloc(e);

done:
    return e->nr;
}

static int res__chain(ti_res_t * res, cleri_node_t * nd, ex_t * e)
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
        if (res__function(res, node, e))
            return e->nr;
        break;
    case CLERI_GID_ASSIGNMENT:
        if (res__assignment(res, node, e))
            return e->nr;
        break;
    case CLERI_GID_NAME:
        if (res__chain_name(res, node, e))
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

    if (node->children && res__index(res, node, e))
        return e->nr;

    child = child->next;
    if (!child)
        goto finish;

    node = child->node              /* optional */
            ->children->node;       /* chain */

    assert (node->cl_obj->gid == CLERI_GID_CHAIN);

    (void) res__chain(res, node, e);

finish:
    return e->nr;
}

static int res__chain_name(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_NAME);
    assert (ti_name_is_valid_strn(nd->str, nd->len));

    ti_name_t * name;
    ti_val_t * val;
    ti_thing_t * thing;

    if (!res_is_thing(res))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no property `%.*s`",
                res_tp_str(res),
                (int) nd->len, nd->str);
        return e->nr;
    }

    thing = res_get_thing(res);

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

    if (ti_scope_push_name(&res->scope, name, val))
        ex_set_alloc(e);

    res_rval_destroy(res);

    return e->nr;
}

static int res__f_blob(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    int n_blobs = res->blobs ? res->blobs->n : 0;
    int64_t idx;

    if (res_get_thing(res) != res->db->root)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `blob`",
                res_tp_str(res));
        return e->nr;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `blob` takes 1 argument but %d were given", n);
        return e->nr;
    }

    if (ti_res_scope(res, nd->children->node, e))
        return e->nr;

    if (res->rval->tp != TI_VAL_INT)
    {
        ex_set(e, EX_BAD_DATA,
                "function `blob` expects argument 1 to be of type `%s` "
                "but got `%s`", ti_val_tp_str(TI_VAL_INT), res_tp_str(res));
        return e->nr;
    }

    idx = res->rval->via.int_;

    if (idx < 0)
        idx += n_blobs;

    if (idx < 0 || idx >= n_blobs)
    {
        ex_set(e, EX_INDEX_ERROR, "blob index out of range");
        return e->nr;
    }

    /* clearing the previous value is not required since this was an integer */
    if (ti_val_set(res->rval, TI_VAL_RAW, vec_get(res->blobs, idx)))
        ex_set_alloc(e);

    return e->nr;
}

static int res__f_endswith(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * val = res_get_val(res);
    _Bool from_rval = val == res->rval;
    _Bool endswith;

    if (!val || !ti_val_is_raw(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `endswith` %p !!!",
                res_tp_str(res), val);
        return e->nr;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `endswith` takes 1 argument but %d were given", n);
        return e->nr;
    }

    res->rval = NULL;

    if (ti_res_scope(res, nd->children->node, e))
        goto done;

    if (!ti_val_is_raw(res->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `endswith` expects argument 1 to be of type `%s` "
                "but got `%s`",
                ti_val_tp_str(TI_VAL_RAW),
                ti_val_str(res->rval));
        goto done;
    }

    endswith = ti_val_endswith(val, res->rval);

    (void) res_rval_clear(res);

    ti_val_set_bool(res->rval, endswith);

done:
    if (from_rval)
        ti_val_destroy(val);
    return e->nr;
}

static int res__f_filter(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * retval = NULL;
    ti_val_t * arrowval = NULL, * iterval = res_get_val(res);
    cleri_node_t * arrow_nd;
    _Bool from_rval = iterval == res->rval;
    ti_thing_t * iter_thing = iterval ? NULL : res_get_thing(res);

    if (iterval && (!ti_val_is_iterable(iterval) || iterval->tp == TI_VAL_RAW))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `filter` %p",
                res_tp_str(res), iterval);
        return e->nr;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `filter` takes 1 argument but %d were given", n);
        return e->nr;
    }

    /* clear res->rval (bug keep the value)
     * from here need need to clean iterval in case of an error */
    res->rval = NULL;
    if (ti_res_scope(res, nd->children->node, e))
        goto failed;

    if (res->rval->tp != TI_VAL_ARROW)
    {
        ex_set(e, EX_BAD_DATA,
                "function `filter` expects an `%s` but got type `%s` instead",
                ti_val_tp_str(TI_VAL_ARROW),
                ti_val_tp_str(res->rval->tp));
        goto failed;
    }

    arrowval = res->rval;
    res->rval = NULL;

    if (ti_scope_local_from_arrow(res->scope, arrowval->via.arrow, e))
        goto failed;

    arrow_nd = arrowval->via.arrow;

    if (iter_thing)
    {
        ti_thing_t * thing = ti_thing_create(0, res->db->things);
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
            for (vec_each(res->scope->local, ti_prop_t, prop), ++n)
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
                    ti_val_set_undefined(&prop->val);
                }
            }

            if (ti_res_scope(res, ti_arrow_scope_nd(arrow_nd), e))
                goto failed;

            if (ti_val_as_bool(res->rval))
            {
                ti_incref(p->name);
                if (ti_thing_setv(thing, p->name, &p->val))
                    goto failed;
            }

            res_rval_weak_destroy(res);
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
                for (vec_each(res->scope->local, ti_prop_t, prop), ++n)
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
                        ti_val_set_undefined(&prop->val);
                    }
                }

                if (ti_res_scope(res, ti_arrow_scope_nd(arrow_nd), e))
                    goto failed;

                if (ti_val_as_bool(res->rval))
                {
                    ti_val_t * dup = ti_val_dup(v);
                    if (!dup)
                        goto failed;
                    VEC_push(retvec, dup);
                }
                res_rval_destroy(res);
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

            for (vec_each(iterval->via.things, ti_thing_t, t), ++idx)
            {
                size_t n = 0;
                for (vec_each(res->scope->local, ti_prop_t, prop), ++n)
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
                        ti_val_set_undefined(&prop->val);
                    }
                }

                if (ti_res_scope(res, ti_arrow_scope_nd(arrow_nd), e))
                    goto failed;

                if (ti_val_as_bool(res->rval))
                {
                    ti_incref(t);
                    VEC_push(retvec, t);
                }
                res_rval_destroy(res);
            }
            (void) vec_shrink(&retval->via.array);
        }
    }

    assert (res->rval == NULL);
    res->rval = retval;

    goto done;

failed:
    ti_val_destroy(retval);
    if (!e->nr)  /* all not set errors are allocation errors */
        ex_set_alloc(e);

    if (from_rval)
        ti_val_destroy(iterval);
done:
    ti_val_destroy(arrowval);
    return e->nr;
}

static int res__f_get(ti_res_t * res, cleri_node_t * nd, ex_t * e)
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

    if (!res_is_thing(res))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `get`",
                res_tp_str(res));
        return e->nr;
    }
    thing = res_get_thing(res);

    n = langdef_nd_n_function_params(nd);
    if (!n)
    {
        ex_set(e, EX_BAD_DATA,
                "function `get` requires at least 1 argument but 0 "
                "were given");
        return e->nr;
    }

    if (!ti_manages_id(thing->id))
    {
        collect_attrs = omap_get(res->collect, thing->id);
        if (!collect_attrs)
        {
            collect_attrs = vec_new(1);
            if (!collect_attrs ||
                omap_add(res->collect, thing->id, collect_attrs))
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
    res_rval_destroy(res);

    for (n = 0; child; child = child->next->next)
    {
        ++n;
        assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);

        if (ti_res_scope(res, child->node, e))
            goto failed;

        assert (res->rval);

        if (!ti_val_is_valid_name(res->rval))
        {
            if (ti_val_is_raw(res->rval))
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
                        ti_val_tp_str(TI_VAL_RAW), n, res_tp_str(res));
            }
            goto failed;
        }

        name = ti_names_get_from_val(res->rval);
        if (!name)
            goto failed;

        ti_val_clear(res->rval);

        if (collect_attrs)
        {
            prop = ti_prop_create(name, TI_VAL_NIL, NULL);
            if (!prop || vec_push(&collect_attrs, prop))
            {
                ti_prop_destroy(prop);
                ex_set_alloc(e);
                goto failed;
            }
            ti_val_weak_set(res->rval, TI_VAL_ATTR, prop);
        }
        else
        {
            prop = ti_thing_attr_get(thing, name);
            if (!prop)
                ti_val_set_nil(res->rval);
            else
                ti_val_weak_set(res->rval, TI_VAL_ATTR, prop);
        }

        VEC_push(ret_array, res->rval);
        res->rval = NULL;

        if (!child->next)
        {
            force_as_array = false;
            break;
        }
    }

    assert (ret_array->n >= 1);

    if (n > 1 || force_as_array)
    {
        if (res_rval_clear(res))
        {
            ex_set_alloc(e);
            goto failed;
        }

        ti_val_weak_set(res->rval, TI_VAL_TUPLE, ret_array);
        goto done;
    }

    res_rval_destroy(res);
    value = vec_pop(ret_array);
    res->rval = value;

    /* bubble down to failed for vec cleanup */
    assert (!e->nr);
    assert (!ret_array->n);

failed:
    vec_destroy(ret_array, (vec_destroy_cb) ti_val_destroy);

done:
    return e->nr;
}

static int res__f_id(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    int64_t thing_id;
    ti_thing_t * thing;

    if (!res_is_thing(res))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `id`",
                res_tp_str(res));
        return e->nr;
    }
    thing = res_get_thing(res);

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `id` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    thing_id = thing->id;

    if (res_rval_clear(res))
        ex_set_alloc(e);
    else
        ti_val_set_int(res->rval, thing_id);

    return e->nr;
}

static int res__f_lower(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * val = res_get_val(res);
    ti_raw_t * lower;

    if (!val || !ti_val_is_raw(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `lower` %p !!!",
                res_tp_str(res), val);
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
    if (!lower || res_rval_clear(res))
        ex_set_alloc(e);
    else
        ti_val_weak_set(res->rval, TI_VAL_RAW, lower);

    return e->nr;
}

static int res__f_thing(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    int n;
    _Bool force_as_array = true;
    vec_t * things;
    ti_thing_t * thing;
    cleri_children_t * child = nd->children;    /* first in argument list */

    if (res_get_thing(res) != res->db->root)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `thing`",
                res_tp_str(res));
        return e->nr;
    }

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

        res_rval_destroy(res);
        if (ti_res_scope(res, child->node, e))
            goto failed;

        assert (res->rval);

        if (res->rval->tp != TI_VAL_INT)
        {
            ex_set(e, EX_BAD_DATA,
                    "function `thing` only accepts `int` arguments, but "
                    "argument %d is of type `%s`", n, res_tp_str(res));
            goto failed;
        }

        thing = ti_db_thing_by_id(res->db, res->rval->via.int_);
        if (!thing)
        {
            ex_set(e, EX_INDEX_ERROR,
                    "database `%.*s` has no `thing` with id `%"PRId64"`",
                    (int) res->db->name->n,
                    (char *) res->db->name->data,
                    res->rval->via.int_);
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
        if (res_rval_clear(res))
        {
            ex_set_alloc(e);
            goto failed;
        }

        ti_val_weak_set(res->rval, TI_VAL_THINGS, things);
        goto done;
    }

    res_rval_destroy(res);
    thing = vec_pop(things);

    assert (thing->ref > 1);
    ti_decref(thing);

    if (ti_scope_push_thing(&res->scope, thing))
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

static int res__f_push(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (res->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    int n;
    cleri_children_t * child = nd->children;    /* first in argument list */
    uint32_t current_n;

    ti_val_t * val = res_get_val(res);
    _Bool from_rval = val == res->rval;

    if (!val || !ti_val_is_mutable_arr(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `push`",
                res_tp_str(res));
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

    if (!from_rval && ti_scope_current_val_in_use(res->scope))
    {
        ex_set(e, EX_BAD_DATA,
                "cannot use function `push` while the array is in use");
        return e->nr;
    }

    current_n = val->via.arr->n;
    if (vec_resize(&val->via.arr, current_n + n))
    {
        ex_set_alloc(e);
        return e->nr;
    }

    assert (child);

    /* we have rval saved to val */
    res->rval = NULL;

    for (n = 0; child; child = child->next->next)
    {
        ++n;
        assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);

        if (ti_res_scope(res, child->node, e))
            goto failed;

        if (res->rval->tp == TI_VAL_THINGS)
        {
            ex_set(e, EX_BAD_DATA,
                    "argument %d is of type `%s` and cannot be "
                    "nested into into another array",
                    n, ti_val_str(res->rval));
            goto failed;
        }

        if (res->rval->tp == TI_VAL_THING)
        {
            /* TODO: I think we can convert back, not sure why I first thought
             * this was not possible. Maybe because of nesting? but that is
             * solved because nested are tuple and therefore not mutable */
            if (val->tp == TI_VAL_ARRAY  && !val->via.array->n)
                val->tp = TI_VAL_THINGS;

            if (val->tp == TI_VAL_ARRAY)
            {
                ex_set(e, EX_BAD_DATA,
                    "argument %d is of type `%s` and cannot be added into an "
                    "array with other types",
                    n,
                    ti_val_str(res->rval));
                goto failed;
            }

            VEC_push(val->via.things, res->rval->via.thing);
            res_rval_weak_destroy(res);
        }
        else
        {
            if (val->tp == TI_VAL_THINGS  && !val->via.things->n)
                val->tp = TI_VAL_ARRAY;

            if (val->tp == TI_VAL_THINGS)
            {
                ex_set(e, EX_BAD_DATA,
                    "argument %d is of type `%s` and cannot be added into an "
                    "array with `things`",
                    n,
                    ti_val_str(res->rval));
                goto failed;
            }

            if (res_assign_val(res, true, e))
                goto failed;

            VEC_push(val->via.array, res->rval);
            res->rval = NULL;
        }

        if (!child->next)
            break;
    }

    if (!from_rval)
    {
        ti_task_t * task;
        assert (res->scope->thing);
        assert (res->scope->name);
        task = res_get_task(res->ev, res->scope->thing, e);
        if (!task)
            goto failed;

        if (ti_task_add_push(task, res->scope->name, val, n))
            goto alloc_err;  /* we do not need to cleanup task, since the task
                                is added to `res->ev->tasks` */
    }

    assert (e->nr == 0);

    goto done;

alloc_err:
    ex_set_alloc(e);

failed:
    res_rval_destroy(res);
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
        res->rval = val;
    return e->nr;
}

static int res__f_ret(ti_res_t * res, cleri_node_t * nd, ex_t * e)
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

    if (res_rval_clear(res))
        ex_set_alloc(e);
    else
        ti_val_set_nil(res->rval);

    return e->nr;
}

static int res__f_set(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (res->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    cleri_node_t * name_nd, * value_nd;
    ti_val_t * name_val;
    ti_task_t * task;
    ti_name_t * name;

    int n;
    ti_thing_t * thing;

    if (!res_is_thing(res))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `set`",
                res_tp_str(res));
        return e->nr;
    }

    thing = res_get_thing(res);
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

    if (ti_res_scope(res, name_nd, e))
        return e->nr;

    if (!ti_val_is_valid_name(res->rval))
    {
        if (ti_val_is_raw(res->rval))
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
                    ti_val_str(res->rval));
        }
        return e->nr;
    }

    name_val = res->rval;
    res->rval = NULL;

    if (ti_res_scope(res, value_nd, e))
        goto finish;

    if (!ti_val_is_settable(res->rval))
    {
        ex_set(e, EX_BAD_DATA, "type `%s` is not settable",
                ti_val_str(res->rval));
        goto finish;
    }

    task = res_get_task(res->ev, thing, e);
    if (!task)
        goto finish;

    name = ti_names_get_from_val(name_val);

    if (!name || ti_task_add_set(task, name, res->rval))
        goto alloc_err;  /* we do not need to cleanup task, since the task
                            is added to `res->ev->tasks` */

    if (ti_manages_id(thing->id))
    {
        if (ti_thing_attr_weak_setv(thing, name, res->rval))
            goto alloc_err;
    }
    else
        ti_val_clear(res->rval);

    ti_val_set_nil(res->rval);

    goto finish;

alloc_err:
    /* we happen to require cleanup of name when alloc_err is used */
    ti_name_drop(name);
    ex_set_alloc(e);

finish:
    ti_val_destroy(name_val);

    return e->nr;
}

static int res__f_startswith(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * val = res_get_val(res);
    _Bool from_rval = val == res->rval;
    _Bool startswith;

    if (!val || !ti_val_is_raw(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `startswith` %p !!!",
                res_tp_str(res), val);
        return e->nr;
    }

    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `startswith` takes 1 argument but %d were given", n);
        return e->nr;
    }

    res->rval = NULL;

    if (ti_res_scope(res, nd->children->node, e))
        goto done;

    if (!ti_val_is_raw(res->rval))
    {
        ex_set(e, EX_BAD_DATA,
                "function `startswith` expects argument 1 to be of type `%s` "
                "but got `%s`",
                ti_val_tp_str(TI_VAL_RAW),
                ti_val_str(res->rval));
        goto done;
    }

    startswith = ti_val_startswith(val, res->rval);

    (void) res_rval_clear(res);

    ti_val_set_bool(res->rval, startswith);

done:
    if (from_rval)
        ti_val_destroy(val);
    return e->nr;
}

static int res__f_unset(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (res->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    cleri_node_t * name_nd;
    ti_task_t * task;
    ti_name_t * name;
    ti_thing_t * thing;

    if (!res_is_thing(res))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `unset`",
                res_tp_str(res));
        return e->nr;
    }

    thing = res_get_thing(res);
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

    if (ti_res_scope(res, name_nd, e))
        return e->nr;

    if (!ti_val_is_valid_name(res->rval))
    {
        if (ti_val_is_raw(res->rval))
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
                    ti_val_str(res->rval));
        }
        return e->nr;
    }

    task = res_get_task(res->ev, thing, e);
    if (!task)
        goto finish;

    name = ti_names_get_from_val(res->rval);

    if (!name || ti_task_add_unset(task, name))
        goto alloc_err;  /* we do not need to cleanup task, since the task
                            is added to `res->ev->tasks` */

    if (ti_manages_id(thing->id))
        ti_thing_attr_unset(thing, name);

    ti_val_clear(res->rval);
    ti_val_set_nil(res->rval);

    goto finish;

alloc_err:
    ex_set_alloc(e);

finish:
    ti_name_drop(name);
    return e->nr;
}

static int res__f_upper(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    ti_val_t * val = res_get_val(res);
    ti_raw_t * upper;

    if (!val || !ti_val_is_raw(val))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `upper` %p !!!",
                res_tp_str(res), val);
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
    if (!upper || res_rval_clear(res))
        ex_set_alloc(e);
    else
        ti_val_weak_set(res->rval, TI_VAL_RAW, upper);

    return e->nr;
}

static int res__function(ti_res_t * res, cleri_node_t * nd, ex_t * e)
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
        return res__f_blob(res, params, e);
    case CLERI_GID_F_ENDSWITH:
        return res__f_endswith(res, params, e);
    case CLERI_GID_F_FILTER:
        return res__f_filter(res, params, e);
    case CLERI_GID_F_GET:
        return res__f_get(res, params, e);
    case CLERI_GID_F_ID:
        return res__f_id(res, params, e);
    case CLERI_GID_F_LOWER:
        return res__f_lower(res, params, e);
    case CLERI_GID_F_THING:
        return res__f_thing(res, params, e);
    case CLERI_GID_F_PUSH:
        return res__f_push(res, params, e);
    case CLERI_GID_F_RET:
        return res__f_ret(res, params, e);
    case CLERI_GID_F_SET:
        return res__f_set(res, params, e);
    case CLERI_GID_F_STARTSWITH:
        return res__f_startswith(res, params, e);
    case CLERI_GID_F_UNSET:
        return res__f_unset(res, params, e);
    case CLERI_GID_F_UPPER:
        return res__f_upper(res, params, e);
    }

    ex_set(e, EX_INDEX_ERROR,
            "type `%s` has no function `%.*s`",
            res_tp_str(res),
            fname->len,
            fname->str);

    return e->nr;
}

static int res__index(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_INDEX);
    assert (nd->children);

    cleri_children_t * child;
    cleri_node_t * node;
    ti_val_t * val;
    int64_t idx;
    ssize_t n;

    val =  res_get_val(res);
    if (!val)
    {
        /* res is of thing type */
        ex_set(e, EX_BAD_DATA, "type `%s` is not indexable", res_tp_str(res));
        return e->nr;
    }

    /* multiple indexes are possible, for example x[0][0] */
    for (child = nd->children; child; child = child->next)
    {
        node = child->node              /* sequence  [ int ]  */
            ->children->next->node;     /* int */

        assert (node->cl_obj->gid == CLERI_GID_T_INT);

        idx = strx_to_int64n(node->str, node->len);

        switch (val->tp)
        {
        case TI_VAL_RAW:
            n = val->via.raw->n;
            break;
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
                if (res_rval_clear(res))
                {
                    ex_set_alloc(e);
                    return e->nr;
                }
                ti_val_set_int(res->rval, c);
            }
            break;
        case TI_VAL_ARRAY:
        case TI_VAL_TUPLE:
            {
                ti_val_t * v = vec_get(val->via.array, idx);
                ti_val_enum tp = v->tp;
                void * data = v->via.nil;
                v->tp = TI_VAL_NIL;
                /* v will now potentially be destroyed,
                 * but data and type are saved */
                if (    res_rval_clear(res) ||
                        ti_val_set(res->rval, tp, data))
                {
                    ex_set_alloc(e);
                    return e->nr;
                };
            }
            break;
        case TI_VAL_THINGS:
            {
                ti_thing_t * thing = vec_get(val->via.things, idx);
                ti_incref(thing);
                if (    res_rval_clear(res) ||
                        ti_scope_push_thing(&res->scope, thing))
                {
                    ex_set_alloc(e);
                    return e->nr;
                }
                /* weak set because incref has been done */
                ti_val_weak_set(res->rval, TI_VAL_THING, thing);
            }
            break;
        default:
            assert (0);
            return -1;
        }
        val = res->rval;
    }
    return e->nr;
}

static int res__operations(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    ti_val_t * a_val;
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
    assert (res->rval == NULL);

    switch (nd->cl_obj->gid)
    {
    case CLERI_GID_OPR0_MUL_DIV_MOD:
    case CLERI_GID_OPR1_ADD_SUB:
    case CLERI_GID_OPR2_COMPARE:
        if (res__operations(res, nd->children->node, e))
            return e->nr;
        a_val = res->rval;
        res->rval = NULL;
        if (res__operations(res, nd->children->next->next->node, e))
            break;
        (void) ti_opr_a_to_b(a_val, nd->children->next->node, res->rval, e);
        break;

    case CLERI_GID_OPR3_CMP_AND:
        if (    res__operations(res, nd->children->node, e) ||
                !ti_val_as_bool(res->rval))
            return e->nr;

        res_rval_destroy(res);
        return res__operations(res, nd->children->next->next->node, e);

    case CLERI_GID_OPR4_CMP_OR:
        if (    res__operations(res, nd->children->node, e) ||
                ti_val_as_bool(res->rval))
            return e->nr;

        res_rval_destroy(res);
        return res__operations(res, nd->children->next->next->node, e);

    case CLERI_GID_SCOPE:
        return ti_res_scope(res, nd, e);

    default:
        assert (0);
    }

    ti_val_destroy(a_val);
    return e->nr;
}

static int res__primitives(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_PRIMITIVES);
    assert (!e->nr);

    cleri_node_t * node = nd            /* choice */
            ->children->node;           /* false, nil, true, undefined,
                                           int, float, string */

    if (res_rval_clear(res))
    {
        ex_set_alloc(e);
        return e->nr;
    }

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_T_FALSE:
        ti_val_set_bool(res->rval, false);
        break;
    case CLERI_GID_T_NIL:
        ti_val_set_nil(res->rval);
        break;
    case CLERI_GID_T_TRUE:
        ti_val_set_bool(res->rval, true);
        break;
    case CLERI_GID_T_UNDEFINED:
        ti_val_set_undefined(res->rval);
        break;
    case CLERI_GID_T_INT:
        ti_val_set_int(res->rval, strx_to_int64n(node->str, node->len));
        break;
    case CLERI_GID_T_FLOAT:
        ti_val_set_float(res->rval, strx_to_doublen(node->str, node->len));
        break;
    case CLERI_GID_T_STRING:
        {
            ti_raw_t * r = ti_raw_from_ti_string(node->str, node->len);
            if (!r)
            {
                ex_set_alloc(e);
                return e->nr;
            }
            ti_val_weak_set(res->rval, TI_VAL_RAW, r);
        }
        break;
    }
    return e->nr;
}

/* changes scope->name and/or scope->thing */
static int res__scope_name(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_NAME);
    assert (ti_name_is_valid_strn(nd->str, nd->len));
    assert (ti_scope_is_thing(res->scope));
    assert (res_is_thing(res));

    ti_name_t * name;
    ti_val_t * val;

    name = ti_names_weak_get(nd->str, nd->len);
    val = name ? ti_scope_find_local_val(res->scope, name) : NULL;

    if (!val && name)
        val = ti_thing_get(res->db->root, name);

    if (!val)
    {
        ex_set(e, EX_INDEX_ERROR,
                "property `%.*s` is undefined",
                (int) nd->len, nd->str);
        return e->nr;
    }

    if (ti_scope_push_name(&res->scope, name, val))
        ex_set_alloc(e);

    res_rval_destroy(res);

    return e->nr;
}

static int res__scope_thing(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    /*
     * Sequence('{', List(Sequence(name, ':', scope)), '}')
     */
    assert (e->nr == 0);
    assert (res_is_thing(res));

    ti_thing_t * thing;
    cleri_children_t * child;
    size_t max_props = res->db->quota->max_props;

    res_rval_destroy(res);

    thing = ti_thing_create(0, res->db->things);
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
                    "maximum properties quota of %u has been reached, "
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

        if (ti_res_scope(res, scope, e))
            goto err;

        assert (res->rval);

        ti_thing_weak_setv(thing, name, res->rval);
        res_rval_weak_destroy(res);

        if (!child->next)
            break;
    }

    if (ti_scope_push_thing(&res->scope, thing))
        goto alloc_err;

    if (res_rval_clear(res))
        goto alloc_err;

    ti_val_weak_set(res->rval, TI_VAL_THING, thing);

    goto done;

alloc_err:
    ex_set_alloc(e);
err:
    ti_thing_drop(thing);
done:
    return e->nr;
}








