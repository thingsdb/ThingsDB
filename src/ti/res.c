/*
 * ti/res.c - query response
 */
#include <assert.h>
#include <ti/res.h>
#include <ti.h>
#include <langdef/langdef.h>
#include <langdef/nd.h>
#include <stdlib.h>
#include <util/strx.h>


static int res__index(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__scope_function(ti_res_t * res, cleri_node_t * nd, ex_t * e);
static int res__f_id(ti_res_t * res, cleri_node_t * nd, ex_t * e);

ti_res_t * ti_res_create(ti_db_t * db)
{
    assert (db && db->root);

    ti_res_t * res = malloc(sizeof(ti_res_t));
    if (!res)
        return NULL;
    res->db = db;  /* a borrowed reference since the query has one */
    res->collect = imap_create();
    res->val = ti_val_create(TI_VAL_THING, db->root);
    if (!res->val || !res->collect)
    {
        ti_res_destroy(res);
        return NULL;
    }
    return res;
}

void ti_res_destroy(ti_res_t * res)
{
    imap_destroy(res->collect, NULL);  /* TODO: maybe we do need a callback? */
    ti_val_destroy(res->val);
    free(res);
}

int res_scope(ti_res_t * res, cleri_node_t * nd, ex_t * e)
{
    assert (nd->cl_obj->gid == CLERI_GID_SCOPE);

    cleri_children_t * child = nd           /* sequence */
                    ->children;             /* first child, choice */

    cleri_node_t * node = child->node       /* choice */
            ->children->node;               /* primitives, function, identifier,
                                               thing, array, compare */

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
        res__scope_function(res, node, e);

        break;
    case CLERI_GID_ASSIGNMENT:
        /*
         * assignment
         *
         */

        break;
    case CLERI_GID_IDENTIFIER:
        /*
         * res->thing = parent or null
         * res->tp = undefined if not found in parent
         * res->value = union (actual value)
         *
         */

        break;
    case CLERI_GID_THING:
        /*
         *
         */
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
        goto finish;

    node = child->node              /* optional */
            ->children->node;       /* chain */

    assert (node->cl_obj->gid == CLERI_GID_CHAIN);


finish:
    return 0;
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
                ti_grab(thing);
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

static int res__scope_function(ti_res_t * res, cleri_node_t * nd, ex_t * e)
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
    {
        ex_set_alloc(e);
    }

    return e->nr;
}



