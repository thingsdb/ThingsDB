/*
 * ti/res.c
 */
#include <assert.h>
#include <ti/res.h>
#include <ti/names.h>
#include <ti.h>
#include <langdef/langdef.h>
#include <langdef/nd.h>
#include <stdlib.h>
#include <util/strx.h>
#include <util/res.h>

static int res__assignment(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__chain(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__chain_name(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__f_filter(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__f_id(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__f_thing(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__f_push(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__function(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__index(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__primitives(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__scope_name(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__scope_thing(ti_res_t * res, cleri_node_t * nd, ex_t * e);

ti_res_t * ti_res_create(ti_db_t * db)
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
//    TODO : do we need to destroy the val ?
//    for (int i = 0; i < 2; ++i)
//    {
//        if (res->iterprops[i].name)
//        {
//            ti_name_drop(res->iterprops[i].name);
//            ti_val_clear()
//        }
//    }


    ti_scope_leave(&res->scope, NULL);
    ti_val_destroy(res->rval);
    free(res);
}

int ti_res_scope(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_SCOPE);

    _Bool mark_fetch = false;
    cleri_children_t * child = nd           /* sequence */
                    ->children;             /* first child, choice */

    cleri_node_t * node = child->node       /* choice */
            ->children->node;               /* primitives, function,
                                               assignment, name, thing,
                                               array, compare */
    ti_scope_t * current_scope = res->scope;
    ti_scope_t * scope = ti_scope_enter(current_scope, res->db->root);
    if (!scope)
    {
        ex_set_alloc(e);
        return e->nr;
    }
    res->scope = scope;


    switch (node->cl_obj->gid)
    {
    case CLERI_GID_PRIMITIVES:
        if (res__primitives(res, node, e))
            return e->nr;
        break;
    case CLERI_GID_FUNCTION:
        if (res__function(res, node, e))
            return e->nr;
        break;
    case CLERI_GID_ASSIGNMENT:
        if (res__assignment(res, node, e))
            return e->nr;
        break;
    case CLERI_GID_NAME:
        if (res__scope_name(res, node, e))
            return e->nr;
        break;
    case CLERI_GID_THING:
        if (res__scope_thing(res, node, e))
            return e->nr;
        break;
    case CLERI_GID_ARRAY:
        /*
         *
         */
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
    {
        mark_fetch = true;
        goto finish;
    }

    node = child->node              /* optional */
            ->children->node;       /* chain */

    assert (node->cl_obj->gid == CLERI_GID_CHAIN);

    (void) res__chain(res, node, e);

finish:
    if (!res->rval)
    {
        res->rval = ti_scope_to_val(res->scope);
        if (!res->rval)
            ex_set_alloc(e);
    }

    if (mark_fetch)
        ti_val_mark_fetch(res->rval);

    ti_scope_leave(&res->scope, current_scope);
    return e->nr;
}

static int res__assignment(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_ASSIGNMENT);
    assert (res->ev);

    ti_thing_t * thing;
    ti_name_t * name;
    ti_task_t * task;
    size_t max_props = res->db->quota->max_props;
    cleri_node_t * name_nd = nd                 /* sequence */
            ->children->node;                   /* name */

    cleri_node_t * scope_nd = nd                /* sequence */
            ->children->next->next->node;       /* scope */

    if (!res_is_thing(res))
    {
        ex_set(e, EX_BAD_DATA, "cannot assign properties to `%s` type",
                res_tp_str(res));
        return e->nr;
    }
    thing = res_get_thing(res);

    if (thing->props->n == max_props)
    {
        ex_set(e, EX_MAX_QUOTA,
                "maximum properties quota of %u has been reached, "
                "see "TI_DOCS"#quotas", max_props);
        return e->nr;
    }

    name = ti_names_get(name_nd->str, name_nd->len);
    if (!name)
        goto alloc_err;

    /* should also work in chain because then iter must be NULL */
    if (    ti_iter_in_use_name(res->scope->iter, name) ||
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
                "property `%.*s` on thing `$id:%"PRIu64"` is undefined",
                (int) nd->len, nd->str,
                thing->id);
        return e->nr;
    }

    if (ti_scope_push_name(&res->scope, name, val))
        ex_set_alloc(e);

    res_rval_destroy(res);

    return e->nr;
}

static int res__f_filter(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (langdef_nd_is_function_params(nd));

    ti_iter_t * iter;
    ti_val_t * retval;
    ti_val_t * val = res_get_val(res);
    cleri_node_t * scope_nd = nd            /* sequence  <list => scope> */
            ->children->next->next->node;   /* scope */

    if (!val || !ti_val_is_iterable(val) || val->tp == TI_VAL_RAW)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `filter`",
                res_tp_str(res));
        return e->nr;
    }

    if (ti_scope_set_iter_names(res->scope, nd, e))
        return e->nr;

    iter = res->scope->iter;

    switch (val->tp)
    {
    case TI_VAL_ARRAY:
    case TI_VAL_TUPLE:
        {
            int64_t idx = 0;
            vec_t * retvec = vec_new(val->via.arr->n);
            if (!retvec)
                goto alloc_err;

            retval = ti_val_weak_create(TI_VAL_ARRAY, retvec);
            if (!retval)
            {
                vec_destroy(retvec, NULL);
                goto alloc_err;
            }

            for (vec_each(val->via.arr, ti_val_t, v), ++idx)
            {
                ti_val_weak_copy(&iter[0]->val, v);
                ti_val_set_int(&iter[1]->val, idx);

                if (ti_res_scope(res, scope_nd, e))
                    goto failed;

                if (ti_val_as_bool(res->rval))
                {
                    ti_val_t * dup = ti_val_dup(v);
                    if (!dup)
                        goto failed;
                    VEC_push(retvec, dup);
                }
                res_rval_weak_destroy(res);
            }
        }
        break;
    case TI_VAL_THINGS:
        {
            int64_t idx = 0;
            vec_t * retvec = vec_new(val->via.arr->n);
            if (!retvec)
                goto alloc_err;

            retval = ti_val_weak_create(TI_VAL_THINGS, retvec);
            if (!retval)
            {
                vec_destroy(retvec, NULL);
                goto alloc_err;
            }

            for (vec_each(val->via.things, ti_thing_t, t), ++idx)
            {
                ti_val_weak_set(&iter[0]->val, TI_VAL_THING, t);
                ti_val_set_int(&iter[1]->val, idx);

                if (ti_res_scope(res, scope_nd, e))
                    goto failed;

                if (ti_val_as_bool(res->rval))
                {
                    ti_incref(t);
                    VEC_push(retvec, t);
                }
                res_rval_weak_destroy(res);
            }
        }
        break;
    case TI_VAL_THING:
        {
            ti_thing_t * thing = ti_thing_create(0, res->db->things);
            if (!thing)
                goto alloc_err;

            retval = ti_val_weak_create(TI_VAL_THING, thing);
            if (!retval)
            {
                ti_thing_drop(thing);
                goto alloc_err;
            }

            for (vec_each(val->via.thing->props, ti_prop_t, p))
            {
                ti_val_weak_set(&iter[0]->val, TI_VAL_NAME, p->name);
                ti_val_weak_copy(&iter[1]->val, &p->val);

                if (ti_res_scope(res, scope_nd, e))
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
    }

failed:
    ti_val_destroy(retval);
    assert (e->nr);
    goto done;

alloc_err:
    ex_set_alloc(e);

done:
    return e->nr;
}

static int res__f_id(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (langdef_nd_is_function_params(nd));

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

    if (langdef_nd_has_function_params(nd))
    {
        int n = langdef_nd_info_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `id` takes 0 arguments but %d %s given",
                abs(n), n == 1 ? "was" : "were");
        return e->nr;
    }

    thing_id = thing->id;

    if (res_rval_clear(res))
        ex_set_alloc(e);
    else
        ti_val_set_int(res->rval, thing_id);

    return e->nr;
}

static int res__f_thing(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (langdef_nd_is_function_params(nd));

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

    n = langdef_nd_info_function_params(nd);
    if (n <= 0)
    {
        ex_set(e, EX_BAD_DATA,
                "function `thing` requires at least one argument but %s",
                n ? "an `iterator` was given" : "none were given");
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
    assert (langdef_nd_is_function_params(nd));

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

    n = langdef_nd_info_function_params(nd);
    if (n <= 0)
    {
        ex_set(e, EX_BAD_DATA,
                "function `push` requires at least one argument but %s",
                n ? "an `iterator` was given" : "none were given");
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
            if (val->tp == TI_VAL_ARRAY)
            {
                ex_set(e, EX_BAD_DATA,
                    "argument %d is of type `%s` and cannot be added into an "
                    "array once it is used for other types",
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

            if (res->rval->tp == TI_VAL_ARRAY)
                res->rval->tp = TI_VAL_TUPLE;   /* convert to tuple */

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

static int res__function(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (langdef_nd_is_function(nd));

    cleri_node_t * fname, * params;

    fname = nd                      /* sequence */
            ->children->node        /* choice */
            ->children->node;       /* keyword or name node */

    params = nd                             /* sequence */
            ->children->next->next->node    /* choice */
            ->children->node;               /* iterator or arguments */


    switch (fname->cl_obj->gid)
    {
    case CLERI_GID_F_FILTER:
        return res__f_filter(res, params, e);
    case CLERI_GID_F_ID:
        return res__f_id(res, params, e);
    case CLERI_GID_F_THING:
        return res__f_thing(res, params, e);
    case CLERI_GID_F_PUSH:
        return res__f_push(res, params, e);
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
                if (res_rval_clear(res))
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
    val = name ? ti_scope_iter_val(res->scope, name) : NULL;

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
        cleri_node_t * name_nd = child->node     /* sequence */
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








