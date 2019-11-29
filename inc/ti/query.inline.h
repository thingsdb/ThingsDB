/*
 * ti/query.inline.h
 */
#ifndef TI_QUERY_INLINE_H_
#define TI_QUERY_INLINE_H_

#include <ti/qbind.h>
#include <util/vec.h>
#include <ti.h>
#include <ti/query.h>

static inline vec_t * ti_query_access(ti_query_t * query)
{
    return query->qbind.flags & TI_QBIND_FLAG_COLLECTION
            ? query->collection->access
            : query->qbind.flags & TI_QBIND_FLAG_THINGSDB
            ? ti()->access_thingsdb
            : query->qbind.flags & TI_QBIND_FLAG_NODE
            ? ti()->access_node
            : NULL;
}

#endif  /* TI_QUERY_INLINE_H_ */
