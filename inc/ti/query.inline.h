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
#include <ti/timer.t.h>
#include <util/vec.h>

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

static inline vec_t ** ti_query_timers(ti_query_t * query)
{
    return query->collection
            ? &query->collection->timers
            : &ti.timers->timers;
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
    ti_query_run_map[query->with_tp](query);
}

static inline void ti_query_response(ti_query_t * query, ex_t * e)
{
    ti_query_done(query, e, ti_query_done_map[query->with_tp]);
}

static inline uint64_t ti_query_scope_id(ti_query_t * query)
{
    if (query->collection)
        return query->collection->root->id;
    if (query->qbind.flags & TI_QBIND_FLAG_THINGSDB)
        return TI_SCOPE_THINGSDB;
    if (query->qbind.flags & TI_QBIND_FLAG_NODE)
        return TI_SCOPE_NODE;

    assert (0);
    return 0;
}

static inline ti_timer_t * ti_timer_query_alt(ti_query_t * query, ex_t * e)
{
    if (query->with_tp == TI_QUERY_WITH_TIMER)
        return query->with.timer;

    ex_set(e, EX_LOOKUP_ERROR,
        "missing timer; use this function within a timer callback or "
        "try to provide a timer ID as first argument");

    return NULL;
}

#endif  /* TI_QUERY_INLINE_H_ */
