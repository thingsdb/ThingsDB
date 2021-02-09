/*
 * ti/timers.h
 */
#ifndef TI_TIMERS_H_
#define TI_TIMERS_H_

typedef struct ti_timers_s ti_timers_t;

#include <ti/timer.t.h>
#include <uv.h>

typedef void (*ti_timers_cb) (ti_timer_t *);

int ti_timers_create(void);
int ti_timers_start(void);
void ti_timers_stop(void);

struct ti_timers_s
{
    _Bool is_started;
    uint32_t n_loops;       /* count number of loops */
    uv_timer_t * timer;
};

#endif  /* TI_TIMERS_H_ */
