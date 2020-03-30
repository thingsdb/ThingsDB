/*
 * ti/query.inline.h
 */
#ifndef TI_QUERY_INLINE_H_
#define TI_QUERY_INLINE_H_

#include <ti/qbind.h>
#include <util/vec.h>
#include <ti.h>
#include <ti/query.h>
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
        (~query->qbind.flags & TI_QBIND_FLAG_API) &&
        !ti_stream_is_client(query->via.stream)
    );
}

static inline vec_t * ti_query_procedures(ti_query_t * query)
{
    return query->collection
            ? query->collection->procedures
            : ti.procedures;
}

static inline void ti_query_var_drop_gc(ti_prop_t * prop, ti_query_t * query)
{
    ti_query_val_gc(prop->val, query);
    ti_prop_destroy(prop);
}

#endif  /* TI_QUERY_INLINE_H_ */
