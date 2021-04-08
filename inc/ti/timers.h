/*
 * ti/timers.h
 */
#ifndef TI_TIMERS_H_
#define TI_TIMERS_H_

typedef struct ti_timers_s ti_timers_t;

/* minimal repeat value of 30 seconds */
#define TI_TIMERS_MIN_REPEAT 30

#include <ti/timer.t.h>
#include <ti/user.t.h>
#include <ti/varr.t.h>
#include <util/vec.h>
#include <uv.h>

typedef int (*ti_timers_cb) (ti_timer_t *);

int ti_timers_create(void);
int ti_timers_start(void);
void ti_timers_stop(void);
void ti_timers_clear(vec_t ** timers);
void ti_timers_del_user(ti_user_t * user);
vec_t ** ti_timers_from_scope_id(uint64_t scope_id);
ti_varr_t * ti_timers_info(vec_t * timers, _Bool with_full_access);

struct ti_timers_s
{
    _Bool is_started;
    uint32_t n_loops;       /* count number of loops */
    vec_t * timers;         /* ti_timer_t */
    uv_timer_t * timer;
    uv_mutex_t * lock;
    char * stat_fn;
};

#endif  /* TI_TIMERS_H_ */
