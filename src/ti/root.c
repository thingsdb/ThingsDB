/*
 * ti/root.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/dbs.h>
#include <ti/root.h>
#include <ti/auth.h>
#include <ti/users.h>
#include <ti/access.h>
#include <ti/task.h>
#include <ti/opr.h>
#include <util/res.h>
#include <util/strx.h>
#include <langdef/langdef.h>
#include <langdef/nd.h>

static int root__f_database_new(ti_root_t * root, cleri_node_t * nd, ex_t * e);
static int root__f_grant(ti_root_t * root, cleri_node_t * nd, ex_t * e);
static int root__f_user_new(ti_root_t * root, cleri_node_t * nd, ex_t * e);
static int root__function(ti_root_t * root, cleri_node_t * nd, ex_t * e);
static int root__name(ti_root_t * root, cleri_node_t * nd, ex_t * e);
static int root__operations(ti_root_t * root, cleri_node_t * nd, ex_t * e);
static int root__primitives(ti_root_t * root, cleri_node_t * nd, ex_t * e);
int root__rval_clear(ti_root_t * root);
static void root__rval_destroy(ti_root_t * root);

ti_root_t * ti_root_create(ti_event_t * ev, ti_user_t * user)
{
    ti_root_t * root = malloc(sizeof(ti_root_t));
    if (!root)
        return NULL;

    root->rval = NULL;
    root->ev = ev;  /* may be NULL */
    root->user = user;
    root->flags = 0;

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
        return e->nr;
    case CLERI_GID_ARROW:
        ex_set(e, EX_BAD_DATA, "arrow functions are not supported at root");
        return e->nr;
    case CLERI_GID_ASSIGNMENT:
        ex_set(e, EX_BAD_DATA, "assignments are not supported at root");
        return e->nr;
    case CLERI_GID_OPERATIONS:
        /* skip the sequence , jump to the priority list */
        if (root__operations(root, node->children->next->node, e))
            return e->nr;
        break;
    case CLERI_GID_FUNCTION:
        if (nested)
        {
            ex_set(e, EX_BAD_DATA,
                    "root functions are not allowed as arguments");
            return e->nr;
        }
        if (root__function(root, node, e))
            return e->nr;
        break;
    case CLERI_GID_NAME:
        if (root__name(root, node, e))
            return e->nr;
        break;
    case CLERI_GID_PRIMITIVES:
        if (root__primitives(root, node, e))
            return e->nr;
        break;
    case CLERI_GID_THING:
        ex_set(e, EX_BAD_DATA, "things are not supported at root");
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

    ex_set(e, EX_BAD_DATA, "chaining is not supported at root");
    return e->nr;

finish:

    if (!root->rval)
    {
        root->rval = ti_val_create(TI_VAL_NIL, NULL);
        if (!root->rval)
        {
            ex_set_alloc(e);
            goto done;
        }
    }

    if (nots)
    {
        _Bool b = ti_val_as_bool(root->rval);
        ti_val_clear(root->rval);
        ti_val_set_bool(root->rval, (nots & 1) ^ b);
    }

done:
    return e->nr;
}

static int root__f_database_new(ti_root_t * root, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (root->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (root->ev->via.query->stream->via.user);

    cleri_children_t * child;
    cleri_node_t * value_nd = NULL;
    ti_raw_t * db_name;
    ti_db_t * db;
    ti_task_t * task;
    ti_user_t * user = root->ev->via.query->stream->via.user;

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

    task = ti_task_get_task(root->ev, ti()->thing0, e);

    if (!task)
        goto finish;

    if (ti_task_add_database_new(task, db, user))
        ex_set_alloc(e);  /* task cleanup is not required */

finish:
    ti_raw_drop(db_name);
    return e->nr;
}

static int root__f_grant(ti_root_t * root, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (root->ev);
    assert (root->user);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (root->ev->via.query->stream->via.user);

    int n;
    ti_db_t * target;
    ti_user_t * user;
    ti_task_t * task;
    uint64_t mask;

    n = langdef_nd_n_function_params(nd);
    if (n != 3)
    {
        ex_set(e, EX_BAD_DATA,
            "function `grant` requires 3 arguments but %d %s given, "
            "see: "TI_DOCS"#grant",
            n, n == 1 ? "was" : "were");
        return e->nr;
    }

    /* grant target, target maybe NULL for root */
    if (ti_root_scope(root, nd->children->node, e))
        return e->nr;

    assert (e->nr == 0);
    target = ti_dbs_get_by_val(root->rval, e);
    if (e->nr)
        return e->nr;

    /* check for privileges */
    if (ti_access_check_err(
            target ? target->access : ti()->access,
            root->user, TI_AUTH_GRANT, e))
        return e->nr;

    /* grant user */
    ti_val_clear(root->rval);
    if (ti_root_scope(root, nd->children->next->node, e))
        return e->nr;

    if (root->rval->tp != TI_VAL_RAW)
    {
        ex_set(e, EX_BAD_DATA,
            "function `grant` expects argument 2 to be of type `%s` "
            "but got `%s`, see: "TI_DOCS"#grant",
            ti_val_tp_str(TI_VAL_RAW),
            ti_val_str(root->rval));
        return e->nr;
    }

    user = ti_users_get_by_namestrn(
            (const char *) root->rval->via.raw->data,
            root->rval->via.raw->n);
    if (!user)
    {
        ex_set(e, EX_BAD_DATA, "user `%.*s` not found",
                (int) root->rval->via.raw->n,
                (char *) root->rval->via.raw->data);
        return e->nr;
    }

    /* grant mask */
    ti_val_clear(root->rval);
    if (ti_root_scope(root, nd->children->next->next->node, e))
        return e->nr;

    if (root->rval->tp != TI_VAL_INT)
    {
        ex_set(e, EX_BAD_DATA,
            "function `grant` expects argument 3 to be of type `%s` "
            "but got `%s`, see: "TI_DOCS"#grant",
            ti_val_tp_str(TI_VAL_INT),
            ti_val_str(root->rval));
        return e->nr;
    }

    mask = (uint64_t) root->rval->via.int_;

    task = ti_task_get_task(root->ev, ti()->thing0, e);
    if (!task)
        return e->nr;

    if (ti_access_grant(target ? &target->access : &ti()->access, user, mask))
    {
        ex_set_alloc(e);
        return e->nr;
    }

    if (ti_task_add_grant(task, target ? target->root->id : 0, user, mask))
        ex_set_alloc(e);  /* task cleanup is not required */

    return e->nr;
}

static int root__f_user_new(ti_root_t * root, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (root->ev);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);
    assert (root->ev->via.query->stream->via.user);

    char * passstr = NULL;
    int n;
    ti_user_t * nuser;
    ti_raw_t * rname;
    ti_task_t * task;

    n = langdef_nd_n_function_params(nd);
    if (n != 2)
    {
        ex_set(e, EX_BAD_DATA,
            "function `user_new` requires 2 arguments but %d %s given",
            n, n == 1 ? "was" : "were");
        return e->nr;
    }

    if (ti_root_scope(root, nd->children->node, e))
        return e->nr;

    if (!ti_val_is_raw(root->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `user_new` expects argument 1 to be of type `%s` "
            "but got `%s`",
            ti_val_tp_str(TI_VAL_RAW),
            ti_val_str(root->rval));
        return e->nr;
    }

    rname = root->rval->via.raw;
    ti_val_set_undefined(root->rval);

    if (ti_root_scope(root, nd->children->next->node, e))
        goto done;

    if (!ti_val_is_raw(root->rval))
    {
        ex_set(e, EX_BAD_DATA,
            "function `user_new` expects argument 2 to be of type `%s` "
            "but got `%s`",
            ti_val_tp_str(TI_VAL_RAW),
            ti_val_str(root->rval));
        goto done;
    }

    passstr = ti_raw_to_str(root->rval->via.raw);
    if (!passstr)
    {
        ex_set_alloc(e);
        goto done;
    }

    task = ti_task_get_task(root->ev, ti()->thing0, e);
    if (!task)
        goto done;

    nuser = ti_users_create_user(
            (const char *) rname->data, rname->n,
            passstr, e);

    if (!nuser)
        goto done;

    if (ti_task_add_user_new(task, nuser, passstr))
        ex_set_alloc(e);  /* task cleanup is not required */

done:
    free(passstr);
    ti_raw_drop(rname);

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

    if (langdef_nd_match_str(fname, "database_new"))
        return root__f_database_new(root, params, e);

    if (langdef_nd_match_str(fname, "grant"))
        return root__f_grant(root, params, e);

    if (langdef_nd_match_str(fname, "user_new"))
        return root__f_user_new(root, params, e);


    ex_set(e, EX_INDEX_ERROR,
            "`%.*s` is undefined",
            fname->len,
            fname->str);

    return e->nr;
}

static int root__name(ti_root_t * root, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->gid == CLERI_GID_NAME);
    assert (ti_name_is_valid_strn(nd->str, nd->len));

    if (root__rval_clear(root))
    {
        ex_set_alloc(e);
        return e->nr;
    }

    int flags
        = langdef_nd_match_str(nd, "READ") ? TI_AUTH_READ
        : langdef_nd_match_str(nd, "MODIFY") ? TI_AUTH_MODIFY
        : langdef_nd_match_str(nd, "WATCH") ? TI_AUTH_WATCH
        : langdef_nd_match_str(nd, "GRANT") ? TI_AUTH_GRANT
        : langdef_nd_match_str(nd, "FULL") ? TI_AUTH_FULL
        : 0;

    if (flags)
        ti_val_set_int(root->rval, flags);
    else
        ex_set(e, EX_INDEX_ERROR,
                "property `%.*s` is undefined",
                (int) nd->len, nd->str);

    return e->nr;
}

static int root__operations(ti_root_t * root, cleri_node_t * nd, ex_t * e)
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
    assert (root->rval == NULL);

    if (nd->cl_obj->gid == CLERI_GID_SCOPE)
        return ti_root_scope(root, nd, e);

    gid = nd->children->next->node->children->node->cl_obj->gid;

    switch (gid)
    {
    case CLERI_GID_OPR0_MUL_DIV_MOD:
    case CLERI_GID_OPR1_ADD_SUB:
    case CLERI_GID_OPR2_BITWISE_AND:
    case CLERI_GID_OPR3_BITWISE_XOR:
    case CLERI_GID_OPR4_BITWISE_OR:
    case CLERI_GID_OPR5_COMPARE:
        if (root__operations(root, nd->children->node, e))
            return e->nr;
        a_val = root->rval;
        root->rval = NULL;
        if (root__operations(root, nd->children->next->next->node, e))
            break;
        (void) ti_opr_a_to_b(a_val, nd->children->next->node, root->rval, e);
        break;

    case CLERI_GID_OPR6_CMP_AND:
        if (    root__operations(root, nd->children->node, e) ||
                !ti_val_as_bool(root->rval))
            return e->nr;

        root__rval_destroy(root);
        return root__operations(root, nd->children->next->next->node, e);

    case CLERI_GID_OPR7_CMP_OR:
        if (    root__operations(root, nd->children->node, e) ||
                ti_val_as_bool(root->rval))
            return e->nr;

        root__rval_destroy(root);
        return root__operations(root, nd->children->next->next->node, e);

    default:
        assert (0);
    }

    ti_val_destroy(a_val);
    return e->nr;
}

static int root__primitives(ti_root_t * root, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_PRIMITIVES);
    assert (!e->nr);

    cleri_node_t * node = nd            /* choice */
            ->children->node;           /* false, nil, true, undefined,
                                           int, float, string */

    if (root__rval_clear(root))
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

int root__rval_clear(ti_root_t * root)
{
    if (root->rval)
    {
        ti_val_clear(root->rval);
        return 0;
    }
    root->rval = ti_val_create(TI_VAL_UNDEFINED, NULL);
    return root->rval ? 0 : -1;
}

static void root__rval_destroy(ti_root_t * root)
{
    ti_val_destroy(root->rval);
    root->rval = NULL;
}
