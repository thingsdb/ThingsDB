/*
 * ti/timer.t.h
 */
#ifndef TI_TIMER_T_H_
#define TI_TIMER_T_H_

#include <ex.h>
#include <inttypes.h>
#include <ti/closure.t.h>
#include <ti/name.t.h>
#include <ti/pkg.t.h>
#include <ti/user.t.h>
#include <ti/timer.t.h>

typedef struct ti_timer_s ti_timer_t;

struct ti_timer_s
{
    uint32_t ref;
    uint32_t repeat;                /* Repeat every X seconds, 0=no repeat */
    uint64_t id;                    /* Unique ID */
    uint64_t scope_id;              /* Scope ID */
    uint64_t next_run;              /* Next run, UNIX time-stamp in seconds */
    ti_user_t * user;               /* Owner of the timer; TODO: delete user */
    ti_closure_t * closure;         /* Closure to run */
    vec_t * args;                   /* Argument values. TODO: walk type, gc */
    ti_raw_t * doc;                 /* documentation, may be NULL */
    ti_raw_t * def;                 /* formatted definition, may be NULL */
};

#endif /* TI_TIMER_T_H_ */


/*
#define ti_timer_repeat(__timer) \
    (__atomic_load_n(&(__timer)->_repeat, __ATOMIC_SEQ_CST))
#define ti_timer_repeat_add(__timer, __x) \
    (__atomic_add_fetch(&(__timer)->_repeat, (__x), __ATOMIC_SEQ_CST))
#define ti_timer_repeat_set(__timer, __x) \
    (__atomic_store_n(&(__timer)->_repeat, (__x), __ATOMIC_SEQ_CST))

#define ti_timer_next_run(__timer) \
    (__atomic_load_n(&(__timer)->_next_run, __ATOMIC_SEQ_CST))
#define ti_timer_next_run_add(__timer, __x) \
    (__atomic_add_fetch(&(__timer)->_next_run, (__x), __ATOMIC_SEQ_CST))
#define ti_timer_next_run_set(__timer, __x) \
    (__atomic_store_n(&(__timer)->_next_run, (__x), __ATOMIC_SEQ_CST))
*/


/*
 * new_timer(datetime().move('days', 1), || {
 *   // download stuff
 *
 *   del_timer();
 * })
 *
 * del_timer(..);
 * stop_timer(...);
 * stop_timer();  -> only effect for repeating timer
 * set_timer_args([...]);
 *
 * set_timer_name(..);
 * set_timer_repeat(..);
 *
 * timer_again(...);
 * timer_info();
 * timers_info();
 *
 */
