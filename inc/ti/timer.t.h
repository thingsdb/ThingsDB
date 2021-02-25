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
    time_t next_run;                /* Next run, UNIX time-stamp in seconds */
    ti_user_t * user;               /* Owner of the timer; TODO: delete user */
    ti_closure_t * closure;         /* Closure to run */
    vec_t * args;                   /* Argument values. TODO: walk type, gc */
    ti_name_t * name;               /* May be NULL, or name */
    ex_t * e;                       /* Last error or NULL (reset at each run) */
};

#endif /* TI_TIMER_T_H_ */




/*
 * new_timer(datetime().move('days', 1), || {
 *   // download stuff
 *
 *   del_timer();
 * })
 *
 * del_timer('my)timer')
 * set_timer_args(...);
 * set_timer_start(...);
 * timer_info();
 * timers_info();
 *
 */
