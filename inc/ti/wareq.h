/*
 * ti/wareq.h
 */
#ifndef TI_WAREQ_H_
#define TI_WAREQ_H_

typedef struct ti_wareq_s ti_wareq_t;

#include <ti/pkg.h>
#include <ex.h>
#include <ti/collection.h>
#include <ti/stream.h>
#include <ti/scope.h>
#include <util/vec.h>

ti_wareq_t * ti_wareq_create(
        ti_stream_t * stream,
        ti_collection_t * collection,
        const char * task);
ti_wareq_t * ti_wareq_may_create(
        ti_scope_t * scope,
        ti_stream_t * stream,
        ti_pkg_t * pkg,
        const char * task,
        ex_t * e);
void ti_wareq_destroy(ti_wareq_t * wareq);
int ti_wareq_run(ti_wareq_t * wareq);

struct ti_wareq_s
{
    ti_stream_t * stream;               /* with reference */
    ti_collection_t * collection;       /* with reference, or null for root */
    vec_t * thing_ids;
    uv_handle_t * task;
};

static inline void * ti_wareq_id(uint64_t id)
{
    #if TI_IS64BIT
    uintptr_t idp = (uintptr_t) id;
    #else
    uint64_t * idp = malloc(sizeof(uint64_t));
    if (!idp)
        return NULL;

    *idp = (uint64_t) id;
    #endif

    return (void *) idp;
}

#endif  /* TI_WAREQ_H_ */
