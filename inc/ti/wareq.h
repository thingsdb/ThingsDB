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
#include <util/vec.h>

ti_wareq_t * ti_wareq_create(ti_stream_t * stream, const char * task);
void ti_wareq_destroy(ti_wareq_t * wareq);
int ti_wareq_unpack(ti_wareq_t * wareq, ti_pkg_t * pkg, ex_t * e);
int ti_wareq_init(ti_stream_t * stream);
int ti_wareq_run(ti_wareq_t * wareq);

struct ti_wareq_s
{
    ti_stream_t * stream;               /* with reference */
    ti_collection_t * collection;       /* with reference, or null for root */
    vec_t * thing_ids;
    uv_async_t * task;
};

#endif  /* TI_WAREQ_H_ */
