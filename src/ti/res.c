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
static int res__f_id(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__f_thing(ti_res_t * res, cleri_node_t * nd, ex_t * e);
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
    res->val = ti_val_weak_create(TI_VAL_THING, db->root);
    res->ev = NULL;
    res->props = NULL;

    if (!res->val || !res->collect)
    {
        ti_res_destroy(res);
        return NULL;
    }
    return res;
}

void ti_res_destroy(ti_res_t * res)
{
    omap_destroy(res->collect, (omap_destroy_cb) res_destroy_collect_cb);
    ti_val_weak_destroy(res->val);
    assert (res->props == NULL);  /* props should be destroyed */
    free(res);
}

int ti_res_scope(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_SCOPE);

    cleri_children_t * child = nd           /* sequence */
                    ->children;             /* first child, choice */

    cleri_node_t * node = child->node       /* choice */
            ->children->node;               /* primitives, function,
                                               assignment, name, thing,
                                               array, compare */

    /* reset res->thing to root */
    res->thing = res->db->root;
    res->name = NULL;

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_PRIMITIVES:
        if (res__primitives(res, node, e))
            return e->nr;
        break;
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

    if (res__index(res, node, e))
        return e->nr;

    child = child->next;
    if (!child)
    {
        ti_val_mark_fetch(res->val);
        goto finish;
    }

    node = child->node              /* optional */
            ->children->node;       /* chain */

    assert (node->cl_obj->gid == CLERI_GID_CHAIN);

    (void) res__chain(res, node, e);

finish:
    return e->nr;
}

static int res__assignment(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_ASSIGNMENT);
    assert (res->ev);

    ti_name_t * name;
    ti_thing_t * parent;
    ti_task_t * task;
    cleri_node_t * name_nd = nd                 /* sequence */
            ->children->node;                   /* name */

    cleri_node_t * scope = nd                   /* sequence */
            ->children->next->next->node;       /* scope */

    if (res->val->tp != TI_VAL_THING)
    {
        ex_set(e, EX_BAD_DATA, "cannot assign properties to `%s` type",
                ti_val_to_str(res->val));
        return e->nr;
    }

    parent = res->val->via.thing;

    res->val->via.thing = res->db->root;
    if (ti_res_scope(res, scope, e))
        goto done;

    name = ti_names_get(name_nd->str, name_nd->len);
    if (!name)
        goto alloc_err;

    task = res_get_task(res->ev, parent, e);
    if (!task)
        goto done;

    if (ti_task_add_assign(task, name, res->val))
        goto alloc_err;  /* we do not need to cleanup task, since the task
                            is added to `res->ev->tasks` */

    if (ti_thing_weak_setv(parent, name, res->val))
    {
        free(vec_pop(task->subtasks));
        goto alloc_err;
    }

    ti_val_set_nil(res->val);

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

    if (res__index(res, node, e))
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

    if (res->val->tp != TI_VAL_THING)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no property `%.*s`",
                ti_val_to_str(res->val),
                (int) nd->len, nd->str);
        return e->nr;
    }

    thing = res->val->via.thing;

    name = ti_names_weak_get(nd->str, nd->len);
    val = name ? ti_thing_get(thing, name) : NULL;

    if (!val)
    {
        ex_set(e, EX_INDEX_ERROR,
                "thing `$id:%"PRIu64"` has no property `%.*s`",
                thing->id,
                (int) nd->len, nd->str);
        return e->nr;
    }

    if (val->tp == TI_VAL_THING)
    {
        res->thing = val->via.thing;
        res->name = NULL;
    }
    else
    {
        assert (res->thing == res->val->via.thing);
        res->name = name;
    }

    ti_val_clear(res->val);
    if (ti_val_copy(res->val, val))
        ex_set_alloc(e);

    return e->nr;
}

static int res__f_id(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (langdef_nd_is_function_params(nd));

    int64_t thing_id;

    if (res->val->tp != TI_VAL_THING)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `id`",
                ti_val_to_str(res->val));
        return e->nr;
    }

    if (langdef_nd_has_function_params(nd))
    {
        int n = langdef_nd_info_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `id` takes 0 arguments but %d %s given",
                abs(n), n == 1 ? "was" : "were");
        return e->nr;
    }

    thing_id = res->val->via.thing->id;

    ti_val_clear(res->val);
    ti_val_set_int(res->val, thing_id);

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

    if (res->val->via.thing != res->db->root)
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `thing`",
                ti_val_to_str(res->val));
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

        ti_val_clear(res->val);
        (void) ti_val_set(res->val, TI_VAL_THING, res->db->root);

        if (ti_res_scope(res, child->node, e))
            goto failed;

        if (res->val->tp != TI_VAL_INT)
        {
            ex_set(e, EX_BAD_DATA,
                    "function `thing` only accepts `int` arguments, but "
                    "argument %d is of type `%s`", n, ti_val_to_str(res->val));
            goto failed;
        }

        thing = ti_db_thing_by_id(res->db, res->val->via.int_);
        if (!thing)
        {
            ex_set(e, EX_INDEX_ERROR,
                    "database `%.*s` has no `thing` with id `%"PRId64"`",
                    (int) res->db->name->n,
                    (char *) res->db->name->data,
                    res->val->via.int_);
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

    ti_val_clear(res->val);
    res->name = NULL;

    if (n > 1 || force_as_array)
    {
        ti_val_weak_set(res->val, TI_VAL_THINGS, things);
        res->thing = NULL;
        goto done;
    }

    ti_val_weak_set(res->val, TI_VAL_THING, vec_pop(things));
    res->thing = res->val->via.thing;

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
    assert (res->thing);

    int n;
    cleri_children_t * child = nd->children;    /* first in argument list */
    ti_task_t * task;
    ti_val_t tmp_val, * pval = res->val;
    ti_thing_t * pthing = res->thing;
    ti_name_t * pname = res->name;
    uint32_t current_n;

    if (!ti_val_is_array(pval))
    {
        ex_set(e, EX_INDEX_ERROR,
                "type `%s` has no function `push`",
                ti_val_to_str(pval));
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


    current_n = pval->via.arr->n;
    if (vec_resize(&pval->via.arr, current_n + n))
    {
        ex_set_alloc(e);
        return e->nr;
    }

    res->val = &tmp_val;

    assert (child);

    for (n = 0; child; child = child->next->next)
    {
        ++n;
        assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);

        (void) ti_val_set(res->val, TI_VAL_THING, res->db->root);

        if (ti_res_scope(res, child->node, e))
            goto failed;

        if (res->val->tp == TI_VAL_THINGS)
        {
            ex_set(e, EX_BAD_DATA,
                    "argument %d is of type `%s` and cannot be "
                    "nested into into another array",
                    n, ti_val_to_str(res->val));
            goto failed;
        }

        if (res->val->tp == TI_VAL_THING)
        {
            if (pval->tp == TI_VAL_ARRAY)
            {
                ex_set(e, EX_BAD_DATA,
                    "argument %d is of type `%s` and cannot be added into an "
                    "array once it is used for other types",
                    n,
                    ti_val_to_str(res->val));
                goto failed;
            }

            VEC_push(pval->via.things, res->val->via.thing);
            ti_val_set_undefined(res->val);

        }
        else
        {
            ti_val_t * dup;
            if (pval->tp == TI_VAL_THINGS  && !pval->via.things->n)
                pval->tp = TI_VAL_ARRAY;

            if (pval->tp == TI_VAL_THINGS)
            {
                ex_set(e, EX_BAD_DATA,
                    "argument %d is of type `%s` and cannot be added into an "
                    "array with `things`",
                    n,
                    ti_val_to_str(res->val));
                goto failed;
            }

            dup = ti_val_weak_dup(res->val);
            if (!dup)
            {
                ex_set_alloc(e);
                goto failed;
            }

            VEC_push(pval->via.array, dup);
            ti_val_set_undefined(res->val);
        }

        if (!child->next)
            break;
    }

    if (pthing)
    {
        task = res_get_task(res->ev, pthing, e);
        if (!task)
            goto failed;

        if (ti_task_add_push(task, pname, pval, n))
            goto alloc_err;  /* we do not need to cleanup task, since the task
                                is added to `res->ev->tasks` */
    }


    assert (e->nr == 0);
    goto done;

alloc_err:
    ex_set_alloc(e);

failed:
    ti_val_clear(res->val);
    while (--n)
    {
        if (pval->tp == TI_VAL_THINGS)
            ti_thing_drop(vec_pop(pval->via.things));
        else
            ti_val_destroy(vec_pop(pval->via.array));
    }
    (void) vec_shrink(&pval->via.arr);

done:
    res->val = pval;
    res->name = pname;
    res->thing = pthing;
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
    case CLERI_GID_F_ID:
        return res__f_id(res, params, e);
    case CLERI_GID_F_THING:
        return res__f_thing(res, params, e);
    case CLERI_GID_F_PUSH:
        return res__f_push(res, params, e);
    }

    ex_set(e, EX_INDEX_ERROR,
            "type `%s` has no function `%.*s`",
            ti_val_to_str(res->val),
            fname->len,
            fname->str);

    return e->nr;
}

static int res__index(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_INDEX);

    cleri_children_t * child;
    cleri_node_t * node;
    ti_val_t * val;
    int64_t idx;
    ssize_t n;

    val =  res->val;

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
            ex_set(e, EX_BAD_DATA,
                    "type `%s` is not indexable",
                    ti_val_to_str(val));
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
                ti_val_clear(val);
                ti_val_set_int(val, c);
            }
            break;
        case TI_VAL_ARRAY:
            {
                ti_val_t * v = vec_get(val->via.array, idx);
                ti_val_enum tp = v->tp;
                void * data = v->via.nil;
                v->tp = TI_VAL_NIL;
                /* v will now be destroyed, but data and type are saved */
                ti_val_clear(val);
                ti_val_weak_set(val, tp, data);
            }
            break;
        case TI_VAL_THINGS:
            {
                ti_thing_t * thing = vec_get(val->via.things, idx);
                ti_incref(thing);
                ti_val_clear(val);
                ti_val_weak_set(val, TI_VAL_THING, thing);
            }
            break;
        default:
            assert (0);
            return -1;
        }
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

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_T_FALSE:
        ti_val_clear(res->val);
        ti_val_set_bool(res->val, false);
        break;
    case CLERI_GID_T_NIL:
        ti_val_clear(res->val);
        ti_val_set_nil(res->val);
        break;
    case CLERI_GID_T_TRUE:
        ti_val_clear(res->val);
        ti_val_set_bool(res->val, true);
        break;
    case CLERI_GID_T_UNDEFINED:
        ti_val_clear(res->val);
        ti_val_set_undefined(res->val);
        break;
    case CLERI_GID_T_INT:
        ti_val_clear(res->val);
        ti_val_set_int(res->val, strx_to_int64n(node->str, node->len));
        break;
    case CLERI_GID_T_FLOAT:
        ti_val_clear(res->val);
        ti_val_set_float(res->val, strx_to_doublen(node->str, node->len));
        break;
    case CLERI_GID_T_STRING:
        {
            ti_raw_t * r = ti_raw_from_ti_string(node->str, node->len);
            if (!r)
            {
                ex_set_alloc(e);
                return e->nr;
            }
            ti_val_clear(res->val);
            ti_val_weak_set(res->val, TI_VAL_RAW, r);
        }
        break;
    }
    return e->nr;
}

static int res__scope_name(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_NAME);
    assert (ti_name_is_valid_strn(nd->str, nd->len));

    ti_name_t * name;
    ti_val_t * val;

    /* not sure, i think res->val should always contain a thing, but we might
     * just want to look at db->root and skip looking at res->val
     */
    assert (res->val->tp == TI_VAL_THING);

    name = ti_names_weak_get(nd->str, nd->len);
    val = name ? ti_thing_get(res->thing, name) : NULL;

    if (!val && name && res->db->root != res->thing)
        val = ti_thing_get(res->db->root, name);

    if (!val)
    {
        ex_set(e, EX_INDEX_ERROR,
                "property `%.*s` is undefined",
                (int) nd->len, nd->str);
        return e->nr;
    }

    if (val->tp == TI_VAL_THING)
    {
        res->thing = val->via.thing;
        res->name = NULL;
        LOGC("no name");
    }
    else
    {
        assert (res->thing == res->val->via.thing);
        LOGC("set name");
        res->name = name;
    }

    ti_val_clear(res->val);

    if (ti_val_copy(res->val, val))
        ex_set_alloc(e);

    return e->nr;
}

static int res__scope_thing(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    /*
     * Sequence('{', List(Sequence(name, ':', scope)), '}')
     */
    assert (e->nr == 0);
    /*
     * if the parent is not a 'thing' then ti_val_copy(&main, res->val)
     * must also be checked
     */
    assert (nd->cl_obj->gid == CLERI_GID_THING);

    ti_thing_t * thing;
    cleri_children_t * child;
    ti_val_t local;
    size_t max_props = res->db->quota->max_props;

    (void) ti_val_copy(&local, res->val);

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


        ti_thing_weak_setv(thing, name, res->val);
        (void) ti_val_copy(res->val, &local);

        if (!child->next)
            break;
    }

    ti_val_clear(res->val);
    ti_val_weak_set(res->val, TI_VAL_THING, thing);
    res->thing = thing;
    res->name = NULL;

    goto done;

alloc_err:
    ex_set_alloc(e);
err:
    ti_thing_drop(res->thing);
done:
    ti_val_clear(&local);
    return e->nr;
}








