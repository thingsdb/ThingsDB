/*
 * query.c
 */
#include <assert.h>
#include <langdef/nd.h>
#include <langdef/translate.h>
#include <qpack.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/dbs.h>
#include <ti/proto.h>
#include <ti/query.h>
#include <ti/res.h>
#include <ti/task.h>
#include <util/qpx.h>

static void query__investigate_recursive(ti_query_t * query, cleri_node_t * nd);
static void query__task_to_watchers(ti_query_t * query);

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
    query->res_statements = NULL;
    query->ev = NULL;

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
    vec_destroy(query->res_statements, (vec_destroy_cb) ti_res_destroy);
    ti_stream_drop(query->stream);
    ti_db_drop(query->target);
    free(query->querystr);
    free(query->raw);
    free(query);
}

int ti_query_unpack(ti_query_t * query, ex_t * e)
{
    assert (e->nr == 0);
    const char * ebad = "invalid query request, see "TI_DOCS"#query";
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
            query->target = ti_dbs_get_by_qp_obj(&val, e);
            ti_grab(query->target);
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
    query->parseres = cleri_parse2(
            ti()->langdef,
            query->querystr,
            CLERI_FLAG_EXPECTING_DISABLED);  /* only error position */
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
    int nstatements = 0;

    assert (e->nr == 0);

    child = query->parseres->tree   /* root */
            ->children->node        /* sequence <comment, list> */
            ->children->next->node  /* list */
            ->children;             /* first child or NULL */

    while(child)
    {
        ++nstatements;
        query__investigate_recursive(query, child->node);  /* scope */

        if (!child->next)
            break;
        child = child->next->next;                  /* skip delimiter */
    }

    query->res_statements = vec_new(nstatements);
    if (!query->res_statements)
        ex_set_alloc(e);

    return e->nr;
}

void ti_query_run(ti_query_t * query)
{
    cleri_children_t * child;
    ex_t * e = ex_use();

        child = query->parseres->tree   /* root */
            ->children->node        /* sequence <comment, list> */
            ->children->next->node  /* list */
            ->children;             /* first child or NULL */

    if (query->target)
    {
        while (child)
        {
            assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);

            ti_res_t * res = ti_res_create(query->target);
            if (!res)
            {
                ex_set_alloc(e);
                goto done;
            }

            res->ev = query->ev;  /* NULL if no update is required */

            VEC_push(query->res_statements, res);

            ti_res_scope(res, child->node, e);
            if (e->nr)
                goto done;

            assert_log(res->collect->n == 0, "collecting is not implemented");

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

done:
    if (query->ev)
        query__task_to_watchers(query);

    ti_query_send(query, e);
}

void ti_query_send(ti_query_t * query, ex_t * e)
{
    ti_pkg_t * pkg;
    qpx_packer_t * packer;
    if (e->nr)
        goto pkg_err;

    /* TODO: we can probably make an educated guess about the required size */
    packer = qpx_packer_create(65536, 8);
    if (!packer)
        goto alloc_err;

    (void) qp_add_array(&packer);

    /* we should have a result for each statement */
    assert (query->res_statements->n == query->res_statements->sz);

    for (vec_each(query->res_statements, ti_res_t, res))
    {
        assert (res);
        assert (res->rval);
        if (ti_val_to_packer(res->rval, &packer, TI_VAL_PACK_FETCH))
            goto alloc_err;
    }

    if (qp_close_array(packer))
        goto alloc_err;

    pkg = qpx_packer_pkg(packer, TI_PROTO_CLIENT_RES_QUERY);
    pkg->id = query->pkg_id;

    goto finish;

alloc_err:
    if (packer)
        qp_packer_destroy(packer);
    ex_set_alloc(e);

pkg_err:
    pkg = ti_pkg_err(query->pkg_id, e);

finish:

    if (!pkg || ti_clients_write(query->stream, pkg))
    {
        free(pkg);
        log_critical(EX_ALLOC_S);
    }
    ti_query_destroy(query);

}

static void query__investigate_recursive(ti_query_t * query, cleri_node_t * nd)
{
    cleri_children_t * child;

    for (child = nd->children; child; child = child->next)
    {
        switch (child->node->cl_obj->gid)
        {
        case CLERI_GID_FUNCTION:
            child->node = child->node           /* sequence */
                ->children->node                /* choice */
                ->children->node;               /* keyword or name */

            switch (nd->cl_obj->gid)
            {
            case CLERI_GID_F_DEL:
            case CLERI_GID_F_PUSH:
            case CLERI_GID_F_REMOVE:
            case CLERI_GID_F_RENAME:
            case CLERI_GID_F_SET:
            case CLERI_GID_F_UNSET:
                query->flags |= TI_QUERY_FLAG_EVENT;
            }

            /* skip to arguments */
            query__investigate_recursive(
                    query,
                    child->node->children->next->next->node);
            continue;
        case CLERI_GID_ASSIGNMENT:
            query->flags |= TI_QUERY_FLAG_EVENT;
            /* skip to scope */
            query__investigate_recursive(
                    query,
                    child->node->children->next->next->node);
            continue;
        case CLERI_GID_ARROW:
            {
                uint8_t flags = query->flags;
                query->flags = 0;
                query__investigate_recursive(
                        query,
                        child->node->children->next->next->node);

                langdef_nd_flag(child->node,
                        query->flags & TI_QUERY_FLAG_EVENT
                        ? LANGDEF_ND_FLAG_ARROW|LANGDEF_ND_FLAG_ARROW_WSE
                        : LANGDEF_ND_FLAG_ARROW);

                query->flags |= flags;
            }
            continue;
        case CLERI_GID_COMMENT:
        case CLERI_GID_INDEX:
        case CLERI_GID_T_STRING:
        case CLERI_GID_PRIMITIVES:
            /* all with children we can skip */
            continue;
        case CLERI_GID_COMPARE:
            /* skip to prio */
            query__investigate_recursive(
                    query,
                    child->node->children->next->node);
            continue;
        }

        if (child->node->children)
            query__investigate_recursive(query, child->node);
    }
}

static void query__task_to_watchers(ti_query_t * query)
{
    omap_iter_t iter = omap_iter(query->ev->tasks);
    for (omap_each(iter, ti_task_t, task))
    {
        omap_iter_id(iter);
        ti_pkg_t * pkg = ti_task_watch(task);
        free(pkg);
    }
}
