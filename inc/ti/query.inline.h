/*
 * ti/query.inline.h
 */
#ifndef TI_QUERY_INLINE_H_
#define TI_QUERY_INLINE_H_

#include <ti/qbind.h>
#include <util/vec.h>
#include <ti.h>
#include <ti/query.h>
#include <ti/qcache.h>
#include <ti/prop.h>

static inline vec_t * ti_query_access(ti_query_t * query)
{
    return query->qbind.flags & TI_QBIND_FLAG_COLLECTION
            ? query->collection->access
            : query->qbind.flags & TI_QBIND_FLAG_THINGSDB
            ? ti.access_thingsdb
            : query->qbind.flags & TI_QBIND_FLAG_NODE
            ? ti.access_node
            : NULL;
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

static inline void ti_query_destroy_or_return(ti_query_t * query)
{
    if (query && (query->flags & TI_QUERY_FLAG_CACHE))
        ti_qcache_return(query);
    else
        ti_query_destroy(query);
}

static inline void ti_query_run(ti_query_t * query)
{
    switch((ti_query_with_enum) query->with_tp)
    {
    case TI_QUERY_WITH_PARSERES:
        ti_query_run_parseres(query);
        return;
    case TI_QUERY_WITH_PROCEDURE:
        ti_query_run_procedure(query);
        return;
    case TI_QUERY_WITH_FUTURE:
        ti_query_run_future(query);
        return;
    }
}

static inline void ti_query_response(ti_query_t * query, ex_t * e)
{
    if (query->with_tp == TI_QUERY_WITH_FUTURE)
        ti_query_on_then_result(query, e);
    else
        ti_query_send_response(query, e);
}

#endif  /* TI_QUERY_INLINE_H_ */
