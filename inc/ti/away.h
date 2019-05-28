/*
 * ti/away.h
 */
#ifndef TI_AWAY_H_
#define TI_AWAY_H_

typedef struct ti_away_s ti_away_t;

#include <uv.h>
#include <util/vec.h>
#include <ti/stream.h>

int ti_away_create(void);
int ti_away_start(void);
void ti_away_trigger(void);
void ti_away_stop(void);
_Bool ti_away_accept(uint8_t node_id);
_Bool ti_away_is_working(void);
int ti_away_syncer(ti_stream_t * stream, uint64_t first);
void ti_away_syncer_done(ti_stream_t * stream);

struct ti_away_s
{
    uv_work_t * work;
    uv_timer_t * repeat;
    uv_timer_t * waiter;
    vec_t * syncers;            /* weak ti_watch_t for synchronizing */
    uint8_t accept_counter;     /* set when a request is accepted */
    uint8_t status;             /* internal state */
    uint8_t expected_node_id;   /* id in the node range */
};

#endif  /* TI_AWAY_H_ */
