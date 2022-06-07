/*
 * ti/vtask.inline.h
 */
#ifndef TI_VTASK_INLINE_H_
#define TI_VTASK_INLINE_H_

#include <ex.h>
#include <ti/val.inline.h>
#include <ti/vtask.h>
#include <ti/vtask.t.h>
#include <tiinc.h>
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
    ti_val_drop((ti_val_t *) vtask->verr);
    vtask->verr = ti_verror_from_code(EX_CANCELLED);
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
    if (vtask->id == 0)
        ex_set(e, EX_VALUE_ERROR, "empty task");
    else if (vtask->flags & TI_VFLAG_LOCK)
        ex_set(e, EX_OPERATION,
            "cannot use "TI_TASK_ID" while the task is locked",
            vtask->id);
    else
        vtask->flags |= TI_VFLAG_LOCK;
    return e->nr;
}

static inline int ti_vtask_is_nil(ti_vtask_t * vtask, ex_t * e)
{
    if (vtask->id == 0)
        ex_set(e, EX_VALUE_ERROR, "empty task");
    return e->nr;
}

static inline int ti_vtask_is_locked(ti_vtask_t * vtask, ex_t * e)
{
    if (vtask->flags & TI_VFLAG_LOCK)
        ex_set(e, EX_OPERATION,
            "cannot use "TI_TASK_ID" while the task is locked",
            vtask->id);
    return e->nr;
}

static inline void ti_vtask_unlock(ti_vtask_t * vtask)
{
    vtask->flags &= ~TI_VFLAG_LOCK;
}

/*
 * Use only from the master, not when handling a change.
 */
static inline void ti_vtask_again_at(ti_vtask_t * vtask, uint64_t run_at)
{
    vtask->run_at = run_at;
    vtask->flags |= TI_VTASK_FLAG_AGAIN;
}

static inline int ti_vtask_num_args(size_t n, size_t m, ex_t * e)
{
    if (n && n >= m)
        ex_set(e, EX_NUM_ARGUMENTS,
                "got %zu task argument%s while the given closure "
                "takes at most %zu argument%s"DOC_TASK,
                n, n == 1 ? "" : "s",
                m ? m-1 : 0, m == 2 ? "" : "s");
    return e->nr;
}

static inline ti_raw_t * ti_vtask_str(ti_vtask_t * vtask)
{
    return vtask->id
        ? ti_str_from_fmt("task:%"PRIu64, vtask->id)
        : ti_str_from_str("task:nil");
}

#endif /* TI_VTASK_INLINE_H_ */
