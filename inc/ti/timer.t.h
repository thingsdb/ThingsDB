/*
 * ti/timer.t.h
 */
#ifndef TI_TIMER_T_H_
#define TI_TIMER_T_H_

#include <ex.h>
#include <inttypes.h>
#include <ti/pkg.t.h>
#include <ti/timer.t.h>

typedef struct ti_timer_s ti_timer_t;

struct ti_timer_s
{
    uint32_t ref;
    uint64_t id;                    /* Unique ID */
    uint64_t created_at;            /* UNIX time-stamp in seconds */
    uint64_t next_run;              /* Next run, UNIX time-stamp in seconds */
    uint64_t repeat;                /* Repeat every X seconds */
    ti_closure_t * closure;         /* Closure to run */
    vec_t * args;                   /* Argument values. TODO: walk type, gc */
    ti_name_t * name;
    ex_t * e;                       /* Last error or 0 (reset at each run) */
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
