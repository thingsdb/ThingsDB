/*
 * ti/query.inline.h
 */
#ifndef TI_QUERY_INLINE_H_
#define TI_QUERY_INLINE_H_

#include <ti.h>
#include <ti/prop.h>
#include <ti/qbind.h>
#include <ti/qcache.h>
#include <ti/query.h>
#include <ti/vtask.t.h>
#include <ti/vset.h>
#include <ti/varr.inline.h>
#include <util/vec.h>

static inline vec_t * ti_query_access(ti_query_t * query)
{
    return query->qbind.flags & TI_QBIND_FLAG_COLLECTION
            ? query->collection->access
            : query->qbind.flags & TI_QBIND_FLAG_THINGSDB
            ? ti.access_thingsdb
            : ti.access_node;
}

static inline _Bool ti_query_is_fwd(ti_query_t * query)
{
    return (
        (~query->flags & TI_QUERY_FLAG_API) &&
        !ti_stream_is_client(query->via.stream)
    );
}

static inline smap_t * ti_query_procedures(ti_query_t * query)
{
    return query->collection
            ? query->collection->procedures
            : ti.procedures;
}

static inline vec_t ** ti_query_tasks(ti_query_t * query)
{
    return query->collection
            ? &query->collection->vtasks
            : &ti.tasks->vtasks;
}

static inline void ti_query_destroy_or_return(ti_query_t * query)
{
    if (query && (query->flags & TI_QUERY_FLAG_CACHE) && query->with.parseres)
        ti_qcache_return(query);
    else
        ti_query_destroy(query);
}

static inline void ti_query_run(ti_query_t * query)
{
    ti_query_run_map[query->with_tp](query);
}

static inline void ti_query_response(ti_query_t * query, ex_t * e)
{
    ti_query_done(query, e, ti_query_done_map[query->with_tp]);
}

static inline uint64_t ti_query_scope_id(ti_query_t * query)
{
    return query->collection
            ?  query->collection->id
            : (query->qbind.flags & TI_QBIND_FLAG_THINGSDB)
            ? TI_SCOPE_THINGSDB
            : TI_SCOPE_NODE;
}

static inline int ti_query_test_vset_operation(ti_query_t * query, ex_t * e)
{
    if (!query->change && ti_vset_is_stored((ti_vset_t *) query->rval))
        ex_set(e, EX_OPERATION,
                "operation on a stored set; "
                "%s(...)` to enforce a change",
                (query->qbind.flags & TI_QBIND_FLAG_NSE)
                    ? "remove `nse"
                    : "use `wse");
    return e->nr;
}

static inline int ti_query_test_varr_operation(ti_query_t * query, ex_t * e)
{
    if (!query->change && ti_varr_is_stored((ti_varr_t *) query->rval))
        ex_set(e, EX_OPERATION,
                "operation on a stored list; "
                "%s(...)` to enforce a change",
                (query->qbind.flags & TI_QBIND_FLAG_NSE)
                    ? "remove `nse"
                    : "use `wse");
    return e->nr;
}

static inline int ti_query_test_thing_operation(ti_query_t * query, ex_t * e)
{
    if (!query->change && ((ti_thing_t *) query->rval)->id)
        ex_set(e, EX_OPERATION,
                "operation on a stored thing; "
                "%s(...)` to enforce a change",
                (query->qbind.flags & TI_QBIND_FLAG_NSE)
                    ? "remove `nse"
                    : "use `wse");
    return e->nr;
}


#endif  /* TI_QUERY_INLINE_H_ */
