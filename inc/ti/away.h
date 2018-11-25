/*
 * ti/away.h
 */
#ifndef TI_AWAY_H_
#define TI_AWAY_H_

typedef struct ti_away_s ti_away_t;

#include <uv.h>

int ti_away_create(void);
void ti_away_destroy(void);

struct ti_away_s
{
    uv_work_t * work;
    uv_timer_t * timer;
    _Bool is_started;
    _Bool is_running;
    uint8_t id;    /* id in the node range */
};

#endif  /* TI_AWAY_H_ */
