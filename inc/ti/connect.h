/*
 * ti/connect.h
 */
#ifndef TI_CONNECT_H_
#define TI_CONNECT_H_

typedef struct ti_connect_s ti_connect_t;

#include <uv.h>

int ti_connect_create(void);
int ti_connect_start(void);
void ti_connect_stop(void);

struct ti_connect_s
{
    _Bool is_started;
    size_t interval;
    uint32_t n_loops;   /* count number of loops */
    uv_timer_t * timer;
};

#endif  /* TI_CONNECT_H_ */
