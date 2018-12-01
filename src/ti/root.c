/*
 * ti/root.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/dbs.h>
#include <ti/root.h>
#include <ti/auth.h>
#include <ti/access.h>
#include <ti/task.h>
#include <util/res.h>
#include <langdef/langdef.h>
#include <langdef/nd.h>

static int root__chain(ti_root_t * root, cleri_node_t * nd, ex_t * e);
static int root__function(ti_root_t * root, cleri_node_t * nd, ex_t * e);
static int root__new_database(ti_root_t * root, cleri_node_t * nd, ex_t * e);
static int root__name(ti_root_t * root, cleri_node_t * nd, ex_t * e);

ti_root_t * ti_root_create(void)
{
    ti_root_t * root = malloc(sizeof(ti_root_t));
    if (!root)
        return NULL;

    root->flags = 0;
    root->rval = NULL;

    return root;
}

void ti_root_destroy(ti_root_t * root)
{
    if (!root)
        return;

    ti_val_destroy(root->rval);
    free(root);
}

int ti_root_scope(ti_root_t * root, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_SCOPE);

    _Bool nested = root->flags & TI_ROOT_FLAG_NESTED;
    int nots = 0;
    cleri_node_t * node;
    cleri_children_t * nchild, * child = nd         /* sequence */
                    ->children;                     /* first child, not */
    ti_scope_t * current_scope, * scope;

    root->flags |= TI_ROOT_FLAG_NESTED;

    for (nchild = child->node->children; nchild; nchild = nchild->next)
        ++nots;

    child = child->next;
    node = child->node                      /* choice */
            ->children->node;               /* primitives, function,
                                               assignment, name, thing,
                                               array, compare, arrow */

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_ARRAY:
        ex_set(e, EX_BAD_DATA, "arrays are not supported at root");
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

static int root__chain(ti_root_t * root, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_CHAIN);

    cleri_children_t * child = nd           /* sequence */
                    ->children->next;       /* first is .(dot), next choice */

    cleri_node_t * node = child->node       /* choice */
            ->children->node;               /* function, assignment,
                                               name */

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_FUNCTION:
        if (root__function(root, node, e))
            return e->nr;
        break;
    case CLERI_GID_ASSIGNMENT:
//        if (root__assignment(root, node, e))
//            return e->nr;
        break;
    case CLERI_GID_NAME:
        if (root__name(root, node, e))
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

    if (node->children)
    {
        ex_set(e, EX_BAD_DATA, "indexing is not supported at root");
        return e->nr;
    }

    child = child->next;
    if (!child)
        goto finish;

    node = child->node              /* optional */
            ->children->node;       /* chain */

    assert (node->cl_obj->gid == CLERI_GID_CHAIN);

    (void) root__chain(root, node, e);

finish:
    return e->nr;

}

static int root__function(ti_root_t * root, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (langdef_nd_is_function(nd));

    cleri_node_t * fname, * params;

    fname = nd                      /* sequence */
            ->children->node        /* choice */
            ->children->node;       /* keyword or name node */

    params = nd                             /* sequence */
            ->children->next->next->node;   /* list of scope (arguments) */


    switch (root->tp)
    {
    case TI_ROOT_DATABASES:
        return root__new_database(root, params, e);
    }
    ex_set(e, EX_INDEX_ERROR,
            "`%.*s` is undefined",
            fname->len,
            fname->str);

    return e->nr;
}

static int root__new_database(ti_root_t * root, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (root->tp == TI_ROOT_DATABASES);
    assert (root->ev);

    cleri_children_t * child;
    cleri_node_t * value_nd = NULL;
    ti_raw_t * db_name;
    ti_db_t * db;
    ti_task_t * task;
    ti_user_t * user = root->ev->via.query->stream->via.user;

    if (!ti_access_check(ti()->access, user, TI_AUTH_DB_CREATE))
    {
        ex_set(e, EX_FORBIDDEN,
                "access denied (requires `%s`)",
                ti_auth_mask_to_str(TI_AUTH_DB_CREATE));
        return e->nr;
    }


    if (!langdef_nd_fun_has_one_param(nd))
    {
        int n = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `new` takes 1 argument but %d were given", n);
        return e->nr;
    }

    nd = nd                         /* list parameters */
        ->children->node            /* first parameter scope */
        ->children                  /* repeat nots */
        ->next->node                /* choice */
        ->children->node;           /* thing or something else */

    if (nd->cl_obj->gid != CLERI_GID_THING)
    {
        ex_set(e, EX_BAD_DATA,
                "function `new` expects argument 1 to be a new `thing`");
        return e->nr;
    }

    child = nd                                      /* sequence */
                ->children->next->node              /* list */
                ->children;                         /* list items */

    for (; child; child = child->next->next)
    {
        cleri_node_t * name_nd = child->node        /* sequence */
                ->children->node;                   /* name */

        if (value_nd || !langdef_nd_match_str(name_nd, "name"))
        {
            ex_set(e, EX_BAD_DATA,
                    "unexpected property `%.*s`",
                    (int) name_nd->len, name_nd->str);
            return e->nr;
        }

        value_nd = child->node                      /* sequence */
                ->children->next->next->node        /* scope */
                ->children                          /* repeat not's */
                ->next->node                        /* choice */
                ->children->node;                   /* primitives? */

        if (    value_nd->cl_obj->gid != CLERI_GID_PRIMITIVES ||
                value_nd->children->node->cl_obj->gid != CLERI_GID_T_STRING)
        {
            ex_set(e, EX_BAD_DATA,
                    "expected `name` to be a plain `string` property");
            return e->nr;
        }

        value_nd = value_nd->children->node;

        if (!child->next)
            break;
    }

    if (!value_nd)
    {
        ex_set(e, EX_BAD_DATA, "expecting a `name` property");
        return e->nr;
    }

    db_name = ti_raw_from_ti_string(value_nd->str, value_nd->len);
    if (!db_name)
    {
        ex_set_alloc(e);
        return e->nr;
    }

    if (!ti_name_is_valid_strn((const char *) db_name->data, db_name->n))
    {
        ex_set(e, EX_BAD_DATA,
                "database name is invalid, see "TI_DOCS"#names");
        goto finish;
    }

    db = ti_dbs_create_db((const char *) db_name->data, db_name->n, user, e);
    if (!db)
        goto finish;

    task = res_get_task(root->ev, ti()->thing0, e);

    if (!task)
        goto finish;

    if (ti_task_add_new_database(task, db, user))
        ex_set_alloc(e);  /* task cleanup is not required */

finish:
    ti_raw_drop(db_name);
    return e->nr;
}

static int root__name(ti_root_t * root, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_NAME);
    assert (ti_name_is_valid_strn(nd->str, nd->len));

    switch (root->tp)
    {
    case TI_ROOT_UNDEFINED:
        if (langdef_nd_match_str(nd, "databases"))
            root->tp = TI_ROOT_DATABASES;
        else if (langdef_nd_match_str(nd, "users"))
            root->tp = TI_ROOT_USERS;
        else if (langdef_nd_match_str(nd, "nodes"))
            root->tp = TI_ROOT_NODES;
        else if (langdef_nd_match_str(nd, "configuration"))
            root->tp = TI_ROOT_CONFIG;
        else
            ex_set(e, EX_INDEX_ERROR,
                    "property `%.*s` is undefined",
                    (int) nd->len, nd->str);
        break;
    case TI_ROOT_DATABASES:
        root->via.db = ti_dbs_get_by_strn(nd->str, nd->len);
        if (!root->via.db)
        {
            ex_set(e, EX_INDEX_ERROR,
                    "database `%.*s` does not exist",
                    (int) nd->len, nd->str);
            break;
        }
        ti_incref(root->via.db);
        root->tp = TI_ROOT_DATABASE;
    }

    return e->nr;
}

static int root__primitives(ti_root_t * root, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_PRIMITIVES);
    assert (!e->nr);

    cleri_node_t * node = nd            /* choice */
            ->children->node;           /* false, nil, true, undefined,
                                           int, float, string */

    if (res_rval_clear(root))
    {
        ex_set_alloc(e);
        return e->nr;
    }

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_T_FALSE:
        ti_val_set_bool(root->rval, false);
        break;
    case CLERI_GID_T_FLOAT:
        ti_val_set_float(root->rval, strx_to_doublen(node->str, node->len));
        break;
    case CLERI_GID_T_INT:
        ti_val_set_int(root->rval, strx_to_int64n(node->str, node->len));
        break;
    case CLERI_GID_T_NIL:
        ti_val_set_nil(root->rval);
        break;
    case CLERI_GID_T_REGEX:
    {
        ti_regex_t * regex = ti_regex_from_strn(node->str, node->len, e);
        if (!regex)
            return e->nr;
        ti_val_weak_set(root->rval, TI_VAL_REGEX, node->data);
        break;
    }
    case CLERI_GID_T_STRING:
    {
        ti_raw_t * raw = ti_raw_from_ti_string(node->str, node->len);
        if (!raw)
        {
            ex_set_alloc(e);
            return e->nr;
        }
        ti_val_weak_set(root->rval, TI_VAL_RAW, node->data);
        break;
    }
    case CLERI_GID_T_TRUE:
        ti_val_set_bool(root->rval, true);
        break;

    }
    return e->nr;
}
