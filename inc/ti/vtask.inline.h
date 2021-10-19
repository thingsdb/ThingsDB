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

static inline int ti_vtask_lock(ti_vtask_t * vtask, ex_t * e)
{
    if (vtask->run_at == 0)
    {
        ex_set(e, EX_VALUE_ERROR,
            "task Id %"PRIu64" is cancelled",
            vtask->id);
        return e->nr;
    }
    if (vtask->flags & TI_VFLAG_LOCK)
    {
        ex_set(e, EX_OPERATION,
            "cannot use task Id %"PRIu64" while the task is locked",
            vtask->id);
        return e->nr;
    }
    vtask->flags |= TI_VFLAG_LOCK;
    return 0;
}

static inline int ti_vtask_is_locked(ti_vtask_t * vtask, ex_t * e)
{
    if (vtask->flags & TI_VFLAG_LOCK)
        ex_set(e, EX_OPERATION,
            "cannot use task Id %"PRIu64" while the task is locked",
            vtask->id);
    return e->nr;
}

static inline void ti_vtask_unlock(ti_vtask_t * vtask)
{
    vtask->flags &= ~TI_VFLAG_LOCK;
}


#endif /* TI_VTASK_INLINE_H_ */
