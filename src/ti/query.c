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
#include <util/qpx.h>

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
    query->parseres = cleri_parse(ti_get()->langdef, query->querystr);
    if (!query->parseres)
    {
        ex_set_alloc(e);
        goto finish;
    }
    if (!query->parseres->is_valid)
    {
        (void) cleri_parse_strn(
                e->msg,
                EX_MAX_SZ,
                query->parseres,
                &langdef_translate);
        e->nr = EX_QUERY_ERROR;
        goto finish;
    }
finish:
    return e->nr;
}

int ti_query_investigate(ti_query_t * query, ex_t * e)
{
    assert (e->nr == 0);
    /* TODO: WILL UPDATE should be placed if the query will update */
    query->flags |= TI_QUERY_FLAG_WILL_UPDATE;

    return e->nr;
}
