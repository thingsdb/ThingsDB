/*
 * ti/vtask.h
 */
#ifndef TI_VTASK_H_
#define TI_VTASK_H_

#include <ex.h>
#include <ti/closure.t.h>
#include <ti/collection.t.h>
#include <ti/verror.h>
#include <ti/vtask.t.h>
#include <ti/raw.t.h>

ti_vtask_t * ti_vtask_create(
        uint64_t id,
        uint64_t run_at,
        ti_user_t * user,
        ti_closure_t * closure,
        ti_verror_t * verr,     /* may be NULL */
        vec_t * args);
ti_vtask_t * ti_vtask_nil(void);
void ti_vtask_destroy(ti_vtask_t * vtask);
int ti_vtask_run(ti_vtask_t * vtask, ti_collection_t * collection);
void ti_vtask_del(uint64_t vtask_id, ti_collection_t * collection);
ti_raw_t * ti_vtask_str(ti_vtask_t * vtask);
int ti_vtask_to_pk(ti_vtask_t * vtask, msgpack_packer * pk, int options);
int ti_vtask_check_args(vec_t * args, size_t m, _Bool ti_scope, ex_t * e);

#endif /* TI_VTASK_H_ */
