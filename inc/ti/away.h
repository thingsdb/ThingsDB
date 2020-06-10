/*
 * ti/away.h
 */
#ifndef TI_AWAY_H_
#define TI_AWAY_H_

typedef struct ti_away_s ti_away_t;

#include <uv.h>
#include <util/vec.h>
#include <ti/stream.h>
#include <inttypes.h>

int ti_away_create(void);
int ti_away_start(void);
void ti_away_set_away_node_id(uint32_t node_id);
void ti_away_reschedule(void);
void ti_away_stop(void);
_Bool ti_away_accept(uint32_t node_id);
_Bool ti_away_is_working(void);
_Bool ti_away_is_busy(void);
int ti_away_syncer(ti_stream_t * stream, uint64_t first);
void ti_away_syncer_done(ti_stream_t * stream);

struct ti_away_s
{
    vec_t * syncers;            /* weak ti_watch_t for synchronizing */
    uint8_t status;             /* internal state */
    uint32_t away_node_id;      /* schedule based on the node in away mode */
    uint32_t sleep;
};

#endif  /* TI_AWAY_H_ */
