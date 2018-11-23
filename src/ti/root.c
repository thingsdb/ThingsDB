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

    root->tp = TI_ROOT_UNDEFINED;
    root->rval = NULL;

    return root;
}

void ti_root_destroy(ti_root_t * root)
{
    if (!root)
        return;
    ti_val_destroy(root->rval);
    switch (root->tp)
    {
    case TI_ROOT_DATABASE:
        ti_db_drop(root->via.db);
    }
    free(root);
}

int ti_root_scope(ti_root_t * root, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_SCOPE);

    int nots = 0;
    cleri_node_t * node;
    cleri_children_t * nchild, * child = nd           /* sequence */
                    ->children;             /* first child, not */

    for (nchild = child->node->children; nchild; nchild = nchild->next)
        ++nots;

    if (nots)
    {
        ex_set(e, EX_BAD_DATA, "cannot use `not` expressions at root");
        return e->nr;
    }

    child = child->next;
    node = child->node                      /* choice */
            ->children->node;               /* primitives, function,
                                               assignment, name, thing,
                                               array, compare, arrow */

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_ARRAY:
        ex_set(e, EX_BAD_DATA, "cannot create `array` at root");
        return e->nr;
    case CLERI_GID_ARROW:
        ex_set(e, EX_BAD_DATA, "cannot create `arrow-function` at root");
        return e->nr;
    case CLERI_GID_ASSIGNMENT:
        ex_set(e, EX_BAD_DATA, "cannot assign properties to root");
        return e->nr;
    case CLERI_GID_OPERATIONS:
        ex_set(e, EX_BAD_DATA, "cannot use operators at root");
        return e->nr;
    case CLERI_GID_FUNCTION:
        if (root__function(root, node, e))
            return e->nr;
        break;
    case CLERI_GID_NAME:
        if (root__name(root, node, e))
            return e->nr;
        break;
    case CLERI_GID_PRIMITIVES:
        ex_set(e, EX_BAD_DATA, "cannot create `primitives` at root");
        return e->nr;
    case CLERI_GID_THING:
        ex_set(e, EX_BAD_DATA, "cannot create `thing` at root");
        return e->nr;
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

    if (root__chain(root, node, e))
        goto finish;

finish:
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
        if (fname->cl_obj->gid == CLERI_GID_F_NEW)
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
