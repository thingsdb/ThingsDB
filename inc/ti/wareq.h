/*
 * ti/wareq.h
 */
#ifndef TI_WAREQ_H_
#define TI_WAREQ_H_

typedef struct ti_wareq_s ti_wareq_t;

#include <ti/pkg.h>
#include <ti/ex.h>
#include <ti/db.h>
#include <ti/stream.h>
#include <util/vec.h>

ti_wareq_t * ti_wareq_create(ti_stream_t * stream);
void ti_wareq_destroy(ti_wareq_t * wareq);
int ti_wareq_unpack(ti_wareq_t * wareq, ti_pkg_t * pkg, ex_t * e);
int ti_wareq_run(ti_wareq_t * wareq);

struct ti_wareq_s
{
    ti_stream_t * stream;       /* with reference */
    ti_db_t * target;           /* with reference */
    vec_t * thing_ids;
    uv_async_t * task;
};

#endif  /* TI_WAREQ_H_ */
