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

ti_query_t * ti_query_create(const unsigned char * data, size_t n)
{
    ti_query_t * query = malloc(sizeof(ti_query_t));
    if (!query)
        return NULL;

    query->flags = 0;
    query->target = NULL;  /* root */
    query->parseres = NULL;
    query->raw = ti_raw_new(data, n);

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
            ->children->node        /* sequence <list, closing-comment> */
            ->children->node        /* list */
            ->children;             /* first child or NULL */

    query->nstatements = 0;

    while(child)
    {
        ++query->nstatements;
        query__investigate_recursive(
                query,
                child->node                     /* sequence <comment, scope> */
                    ->children->next->node      /* scope */
        );

        if (!child->next)
            break;
        child = child->next->next;
    }

    LOGC("query flags: %d", query->flags);

    return e->nr;
}

int ti_query_run(ti_query_t * query, ex_t * e)
{
    query->target;
}

typedef enum
{
    TI_ROOT_UNDEFINED,
    TI_ROOT_ROOT,
    TI_ROOT_DATABASES,
    TI_ROOT_DATABASES_CREATE,
    TI_ROOT_DATABASES_DATABASE,
    TI_ROOT_USERS,
    TI_ROOT_CONFIG_REDUNDANCY,
    TI_ROOT_CONFIG,
} ti_root_enum;

ti_root_enum * ti_root_by_strn(const char * str, const size_t n)
{
    if (strncmp("databases", str, n) == 0)
        return TI_ROOT_DATABASES;

    if (strncmp("users", str, n) == 0)
        return TI_ROOT_USERS;

    if (strncmp("config", str, n) == 0)
        return TI_ROOT_CONFIG;

    return TI_ROOT_UNDEFINED;
}

static ex_enum query__root(ti_query_t * query, ti_root_enum scope, cleri_node_t * nd, ex_t * e)
{
    cleri_children_t * child;
    cleri_t * obj = nd->cl_obj;

    switch (obj->tp)
    {
    case CLERI_GID_IDENTIFIER:
        switch (scope)
        {
        case TI_ROOT_ROOT:
            switch ((scope = ti_root_by_strn(nd->str, nd->len)))
            {
            case TI_ROOT_DATABASES:
            case TI_ROOT_USERS:
            case TI_ROOT_CONFIG:
                break;
            default:
                ex_set(e, EX_INDEX_ERROR,
                        "`ThingsDB` has no property `%.*s`",
                        nd->len,
                        nd->str);
                return e->nr;
            }
            break;
        case TI_ROOT_DATABASES:
            ex_set(e, EX_INDEX_ERROR,
                    "`ThingsDB.databases` has no property `%.*s`",
                    nd->len,
                    nd->str);
            return e->nr;
        }
        break;
    case CLERI_GID_FUNCTION:
        switch (scope)
        {
        case TI_ROOT_ROOT:
            break;
        case TI_ROOT_DATABASES:


        }
        }
    }

    for (child = nd->children; child; child = child->next)
        if (query__root(query, scope, child->node, e))
            break;
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



