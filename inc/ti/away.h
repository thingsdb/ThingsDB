/*
 * ti/away.h
 */
#ifndef TI_AWAY_H_
#define TI_AWAY_H_

typedef struct ti_away_s ti_away_t;

#include <uv.h>
#include <util/vec.h>

int ti_away_create(void);
int ti_away_start(void);
void ti_away_trigger(void);
void ti_away_stop(void);
_Bool ti_away_is_working(void);

struct ti_away_s
{
    uv_work_t * work;
    uv_timer_t * repeat;
    uv_timer_t * waiter;
    vec_t * syncers;    /* weak ti_watch_t for synchronizing */
    uint8_t flags;      /* internal state flags */
    uint8_t id;         /* id in the node range */
};

#endif  /* TI_AWAY_H_ */
