/*
 * query.c
 */
#include <assert.h>
#include <ti/query.h>
#include <ti.h>
#include <ti/dbs.h>
#include <stdlib.h>
#include <qpack.h>
#include <langdef/translate.h>
#include <langdef/nd.h>
#include <util/qpx.h>

static void query__investigate_recursive(ti_query_t * query, cleri_node_t * nd);

ti_query_t * ti_query_create(ti_stream_t * stream, ti_pkg_t * pkg)
{
    ti_query_t * query = malloc(sizeof(ti_query_t));
    if (!query)
        return NULL;

    query->pkg_id = pkg->id;
    query->flags = 0;
    query->target = NULL;  /* root */
    query->parseres = NULL;
    query->raw = ti_raw_new(pkg->data, pkg->n);
    query->stream = ti_grab(stream);

    if (!query->raw)
    {
        ti_query_destroy(query);
        return NULL;
    }

    return query;
}

void ti_query_destroy(ti_query_t * query)
{
    if (!query)
        return;
    if (query->parseres)
        cleri_parse_free(query->parseres);
    ti_drop_stream(query->stream);
    ti_db_drop(query->target);
    free(query->querystr);
    free(query->raw);
    free(query);
}

int ti_query_unpack(ti_query_t * query, ex_t * e)
{
    assert (e->nr == 0);
    const char * ebad = "invalid query request, see "TI_DOCS"#query_request";
    qp_unpacker_t unpacker;
    qp_obj_t key, val;

    qp_unpacker_init2(&unpacker, query->raw->data, query->raw->n, 0);

    if (!qp_is_map(qp_next(&unpacker, NULL)))
    {
        ex_set(e, EX_BAD_DATA, ebad);
        goto finish;
    }

    while(qp_is_raw(qp_next(&unpacker, &key)))
    {
        if (qp_is_raw_equal_str(&key, "query"))
        {
            if (!qp_is_raw(qp_next(&unpacker, &val)))
            {
                ex_set(e, EX_BAD_DATA, ebad);
                goto finish;
            }
            query->querystr = qpx_obj_raw_to_str(&val);
            if (!query->querystr)
            {
                ex_set_alloc(e);
                goto finish;
            }
            continue;
        }

        if (qp_is_raw_equal_str(&key, "target"))
        {
            (void) qp_next(&unpacker, &val);
            query->target = ti_grab(ti_dbs_get_by_qp_obj(&val, e));
            if (e->nr)
                goto finish;
            /* query->target maybe NULL in case when the target is root */
            continue;
        }

        if (!qp_is_map(qp_next(&unpacker, NULL)))
        {
            ex_set(e, EX_BAD_DATA, ebad);
            goto finish;
        }
    }

finish:
    return e->nr;
}

int ti_query_parse(ti_query_t * query, ex_t * e)
{
    assert (e->nr == 0);
    query->parseres = cleri_parse(ti()->langdef, query->querystr);
    if (!query->parseres)
    {
        ex_set_alloc(e);
        goto finish;
    }
    if (!query->parseres->is_valid)
    {
        int i = cleri_parse_strn(
                e->msg,
                EX_MAX_SZ,
                query->parseres,
                &langdef_translate);

        assert_log(i<EX_MAX_SZ, "expecting >= max size %d>=%d", i, EX_MAX_SZ);

        /* we will certainly not hit the max size, but just to be safe */
        e->n = i < EX_MAX_SZ ? i : EX_MAX_SZ - 1;
        e->nr = EX_QUERY_ERROR;
        goto finish;
    }
finish:
    return e->nr;
}

int ti_query_investigate(ti_query_t * query, ex_t * e)
{
    cleri_children_t * child;

    assert (e->nr == 0);

    child = query->parseres->tree   /* root */
            ->children->node        /* sequence <comment, list> */
            ->children->next->node  /* list */
            ->children;             /* first child or NULL */

    query->nstatements = 0;

    while(child)
    {
        ++query->nstatements;
        query__investigate_recursive(child->node);  /* scope */

        if (!child->next)
            break;
        child = child->next->next;                  /* skip delimiter */
    }

    LOGC("query flags: %d", query->flags);

    return e->nr;
}

int ti_query_run(ti_query_t * query, ex_t * e)
{
    ti_pkg_t * resp;
    cleri_children_t * child;
    qp_packer_t * packer;

    assert (e->nr == 0);

    /* TODO: we can probably make a better guess about the required size */
    packer = qpx_packer_create(65536, 8);
    if (!packer)
        return -1;

    child = query->parseres->tree   /* root */
            ->children->node        /* sequence <comment, list> */
            ->children->next->node  /* list */
            ->children;             /* first child or NULL */

    if (query->target)
    {
        while (child)
        {
            ex_t * e = ex_use();
            ti_res_t * res = ti_res_create(query->target);

            res__scope(res, child->node, e);

            /* TODO: handle res->collect->n which means we first need to
             *       fetch things */
            assert_log(res->collect->n == 0, "collecting is not implemented");

            if (e->nr)
            {
                resp = ti_pkg_err(query->pkg_id, e);
            }
            else
            {
                if (ti_val_to_packer(res->val, packer))
                {
                    /* TODO: cleanup etc ?? */
                    return -1;
                }
                pkg_req = qpx_packer_pkg(packer, TI_PROTO_NODE_REQ_QUERY);

            }
            query->stream

            if (!child->next)
                break;
            child = child->next->next;                  /* skip delimiter */
        }

    }
    else
    {
        /* TODO: implement root */
        assert_log(0, "root queries not implemented yet");
    }
}

static ti_res_t * res__f_fetch(ti_scope_t * scope, cleri_node_t ** nd, ex_t * e)
{
    assert (e->nr == 0);

    cleri_node_t * fname;
    ti_res_t * res;

    fname = (*nd)                   /* sequence */
            ->children->node        /* choice */
            ->children->node;       /* keyword or identifier node */

    switch (scope->parent->tp)
    {
    case TI_VAL_THING:
        ti_manages_id(scope->parent->via.thing_->id)
    }
    ex_set(e, EX_INDEX_ERROR,
            "type `%s` has no function `%.*s`",
            ti_val_to_str(scope->parent),
            fname->len,
            fname->str);
    return NULL;
}

static ti_res_t * res__function(ti_scope_t * scope, cleri_node_t ** nd, ex_t * e)
{
    assert (e->nr == 0);
    assert(langdef_nd_is_function(*nd));

    cleri_node_t * fname;
    ti_res_t * res;

    fname = (*nd)                   /* sequence */
            ->children->node        /* choice */
            ->children->node;       /* keyword or identifier node */

    switch (fname->cl_obj->gid)
    {
    case CLERI_GID_F_FETCH:
        return res__f_fetch(scope, nd, e);
    }

    ex_set(e, EX_INDEX_ERROR,
            "type `%s` has no function `%.*s`",
            ti_val_to_str(scope->parent),
            fname->len,
            fname->str);
    return NULL;
}

static ti_res_t * res__scope(ti_res_t * res, cleri_node_t ** nd, ex_t * e)
{
    assert ((*nd)->cl_obj->gid == CLERI_GID_SCOPE);

    cleri_node_t * node;
    cleri_children_t * child = (*nd)        /* sequence */
                    ->children;
    node = child->node                      /* choice */
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

    case CLERI_GID_FUNCTION:
        /*
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
    case CLERI_GID_ARRAY:
        /*
         *
         */
        break;
    default:
        assert (0);
        return NULL;
    }

    child = child->next;
    if (!child)
        goto finish;

    node = child->node;
    if (node->cl_obj->gid == CLERI_GID_INDEX)
    {
        /* handle index */
        child = child->next;
        if (!child)
            goto finish;

        node = child->node;
    }

    /* handle follow-up */
    node = node                     /* optional */
            ->children->node        /* choice */
            ->children->node;       /* chain or assignment */

    switch (node->cl_obj->gid)
    {
    case CLERI_GID_CHAIN:
        break;
    case CLERI_GID_ASSIGNMENT:
        break;
    default:
        assert (0);
        return NULL;
    }

finish:
    return NULL;
}

static ti_res_t * res__walk(ti_val_t * parent, cleri_node_t ** nd, ex_t * e)
{
    assert (e->nr == 0);
    cleri_children_t * child;
    cleri_t * obj = (*nd)->cl_obj;
    switch (obj->gid)
    {
    case CLERI_GID_IDENTIFIER:
        if (parent->tp == TI_VAL_THING)
        {

        }
        break;
    case CLERI_GID_FUNCTION:
        if (parent->tp == TI_VAL_THING)
        {
            // allow drop etc.
        }
        else if (parent->tp == TI_VAL_THINGS)
        {
            // allow push etc.
        }
    }


    for (child = (*nd)->children; child; child = child->next)
    {
        res__walk(parent, child->node);
    }
}


static void query__investigate_recursive(ti_query_t * query, cleri_node_t * nd)
{
    /*
     * TODO: this is not optimized in any way, but we will do this later since
     *       we might want to perform 'other' tasks at this point.
     */
    cleri_children_t * child;

    if (langdef_nd_is_update_function(nd))
        query->flags |= TI_QUERY_FLAG_WILL_UPDATE;

    for (child = nd->children; child; child = child->next)
        query__investigate_recursive(query, child->node);
}



