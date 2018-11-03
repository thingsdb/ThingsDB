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


static int res__chain(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__chain_identifier(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__f_id(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__function(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__index(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__scope_assignment(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__scope_identifier(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__scope_thing(ti_res_t * res, cleri_node_t * nd, ex_t * e);

ti_res_t * ti_res_create(ti_db_t * db)
{
    assert (db && db->root);

    ti_res_t * res = malloc(sizeof(ti_res_t));
    if (!res)
        return NULL;

    res->db = db;  /* a borrowed reference since the query has one */
    res->collect = imap_create();
    res->val = ti_val_create(TI_VAL_THING, db->root);
    res->ev = NULL;

    if (!res->val || !res->collect)
    {
        ti_res_destroy(res);
        return NULL;
    }
    return res;
}

void ti_res_destroy(ti_res_t * res)
{
    imap_destroy(res->collect, (imap_destroy_cb) res_destroy_collect_cb);
    ti_val_destroy(res->val);
    free(res);
}

int ti_res_scope(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_SCOPE);

    cleri_children_t * child = nd           /* sequence */
                    ->children;             /* first child, choice */

    cleri_node_t * node = child->node       /* choice */
            ->children->node;               /* primitives, function,
                                               assignment, identifier, thing,
                                               array, compare */

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_PRIMITIVES:
        /*
         * res->thing = Set to NULL
         * res->tp = NIL / BOOL / STRING / INT / FLOAT
         * res->value = union (actual value)
         */
        break;
    case CLERI_GID_FUNCTION:
        if (res__function(res, node, e))
            return e->nr;
        break;
    case CLERI_GID_ASSIGNMENT:
        if (res__scope_assignment(res, node, e))
            return e->nr;
        break;
        break;
    case CLERI_GID_IDENTIFIER:
        if (res__scope_identifier(res, node, e))
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

static int res__chain(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_CHAIN);

    cleri_children_t * child = nd           /* sequence */
                    ->children->next;       /* first is .(dot), next choice */

    cleri_node_t * node = child->node       /* choice */
            ->children->node;               /* function, assignment,
                                               identifier */

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_FUNCTION:
        if (res__function(res, node, e))
            return e->nr;
        break;
    case CLERI_GID_ASSIGNMENT:
        /*
         * assignment
         */
        break;
    case CLERI_GID_IDENTIFIER:
        if (res__chain_identifier(res, node, e))
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

static int res__chain_identifier(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_IDENTIFIER);
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
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `id()` takes 0 arguments but %d %s given",
                n, n == 1 ? "was" : "were");
        return e->nr;
    }

    thing_id = res->val->via.thing->id;

    ti_val_clear(res->val);

    if (ti_val_set(res->val, TI_VAL_INT, &thing_id))
        ex_set_alloc(e);

    return e->nr;
}

static int res__function(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert(langdef_nd_is_function(nd));

    cleri_node_t * fname, * params;

    fname = nd                      /* sequence */
            ->children->node        /* choice */
            ->children->node;       /* keyword or identifier node */

    params = nd                             /* sequence */
            ->children->next->next->node    /* choice */
            ->children->node;               /* iterator or arguments */


    switch (fname->cl_obj->gid)
    {
    case CLERI_GID_F_ID:
        return res__f_id(res, params, e);
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
        case TI_VAL_PRIMITIVES:
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
                if (ti_val_set(val, TI_VAL_INT, &c))
                    ex_set_alloc(e);
            }
            break;
        case TI_VAL_PRIMITIVES:
            {
                ti_val_t * v = vec_get(val->via.primitives, idx);
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

static int res__scope_assignment(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_ASSIGNMENT);
    assert (res->val->tp == TI_VAL_THING);
    assert (res->ev);

    ti_name_t * name;
    ti_thing_t * parent;
    ti_task_t * task;
    cleri_node_t * identifier = nd              /* sequence */
            ->children->node;                   /* identifier */

    cleri_node_t * scope = nd                   /* sequence */
            ->children->next->next->node;       /* scope */

    parent = res->val->via.thing;

    res->val->via.thing = ti_grab(res->db->root);
    if (ti_res_scope(res, scope, e))
        goto done;

    name = ti_names_get(identifier->str, identifier->len);
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

    ti_val_weak_set(res->val, TI_VAL_NIL, NULL);

    goto done;

alloc_err:
    ex_set_alloc(e);

done:
    ti_thing_drop(parent);
    return e->nr;
}

static int res__scope_identifier(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_IDENTIFIER);
    assert (ti_name_is_valid_strn(nd->str, nd->len));

    ti_name_t * name;
    ti_val_t * val;
    ti_thing_t * thing;

    /* not sure, i think res->val should always contain a thing, but we might
     * just want to look at db->root and skip looking at res->val
     */
    assert (res->val->tp == TI_VAL_THING);

    thing = res->val->via.thing;

    name = ti_names_weak_get(nd->str, nd->len);
    val = name ? ti_thing_get(thing, name) : NULL;

    if (!val && name && res->db->root != thing)
        val = ti_thing_get(res->db->root, name);

    if (!val)
    {
        ex_set(e, EX_INDEX_ERROR,
                "property `%.*s` is undefined",
                (int) nd->len, nd->str);
        return e->nr;
    }

    ti_val_clear(res->val);

    if (ti_val_copy(res->val, val))
        ex_set_alloc(e);

    return e->nr;
}

static int res__scope_thing(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    /*
     * Sequence('{', List(Sequence(identifier, ':', scope)), '}')
     */
    assert (e->nr == 0);
    /*
     * if the parent is not a 'thing' then ti_val_copy(&main, res->val)
     * must also be checked
     */
    assert (nd->cl_obj->gid == CLERI_GID_THING);

    cleri_children_t * child;
    ti_thing_t * thing;
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
        cleri_node_t * identifier = child->node     /* sequence */
                ->children->node;                   /* identifier */

        cleri_node_t * scope = child->node          /* sequence */
                ->children->next->next->node;       /* scope */

        ti_name_t * name = ti_names_get(identifier->str, identifier->len);
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

    goto done;

alloc_err:
    ex_set_alloc(e);
err:
    ti_thing_drop(thing);
done:
    ti_val_clear(&local);
    return e->nr;
}








