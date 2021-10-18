/*
 * ti/vtask.inline.h
 */
#ifndef TI_VTASK_INLINE_H_
#define TI_VTASK_INLINE_H_

#include <ex.h>
#include <ti/vtask.t.h>
#include <ti/vtask.h>
#include <util/vec.h>

static inline void ti_vtask_drop(ti_vtask_t * vtask)
{
    if (vtask && !--vtask->ref)
        ti_vtask_destroy(vtask);
}

static inline void ti_vtask_unsafe_drop(ti_vtask_t * vtask)
{
    if (!--vtask->ref)
        ti_vtask_destroy(vtask);
}

static inline void ti_vtask_cancel(ti_vtask_t * vtask)
{
    vtask->run_at = 0;
}

static inline ti_vtask_t * ti_vtask_by_id(vec_t * vtasks, uint64_t id)
{
    for (vec_each(vtasks, ti_vtask_t, vtask))
        if (vtask->id == id)
            return vtask;
    return NULL;
}




#endif /* TI_VTASK_INLINE_H_ */
