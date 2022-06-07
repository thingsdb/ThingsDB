/*
 * ti/vtask.t.h
 */
#ifndef TI_VTASK_T_H_
#define TI_VTASK_T_H_

#include <ex.h>
#include <inttypes.h>
#include <ti/closure.t.h>
#include <ti/name.t.h>
#include <ti/pkg.t.h>
#include <ti/user.t.h>
#include <ti/verror.h>
#include <ti/vtask.t.h>

enum
{
    TI_VTASK_FLAG_RUNNING   =1<<0,      /* task is running */
    TI_VTASK_FLAG_AGAIN     =1<<1,      /* task is re-scheduled */
};

typedef struct ti_vtask_s ti_vtask_t;

struct ti_vtask_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    uint16_t _pad1;
    uint64_t id;                    /* Unique ID */
    uint64_t run_at;                /* Run at UNIX time-stamp in seconds */
    ti_user_t * user;               /* Owner of the vtask */
    ti_closure_t * closure;         /* Closure to run */
    ti_verror_t * verr;             /* Last run error status */
    vec_t * args;                   /* Argument values */
};

#endif /* TI_VTASK_T_H_ */


/*
#define ti_vtask_repeat(__vtask) \
    (__atomic_load_n(&(__vtask)->_repeat, __ATOMIC_SEQ_CST))
#define ti_vtask_repeat_add(__vtask, __x) \
    (__atomic_add_fetch(&(__vtask)->_repeat, (__x), __ATOMIC_SEQ_CST))
#define ti_vtask_repeat_set(__vtask, __x) \
    (__atomic_store_n(&(__vtask)->_repeat, (__x), __ATOMIC_SEQ_CST))

#define ti_vtask_next_run(__vtask) \
    (__atomic_load_n(&(__vtask)->_next_run, __ATOMIC_SEQ_CST))
#define ti_vtask_next_run_add(__vtask, __x) \
    (__atomic_add_fetch(&(__vtask)->_next_run, (__x), __ATOMIC_SEQ_CST))
#define ti_vtask_next_run_set(__vtask, __x) \
    (__atomic_store_n(&(__vtask)->_next_run, (__x), __ATOMIC_SEQ_CST))
*/

