/*
 * ti/event.h
 */
#ifndef TI_EVENT_H_
#define TI_EVENT_H_

#include <stdint.h>
#include <sys/time.h>
#include <ti/event.t.h>
#include <ti/thing.t.h>
#include <util/vec.h>
#include <util/logger.h>

ti_event_t * ti_event_create(ti_event_tp_enum tp);
ti_event_t * ti_event_epkg(ti_epkg_t * epkg);
ti_event_t * ti_event_initial(void);
void ti_event_drop(ti_event_t * ev);
int ti_event_append_pkgs(ti_event_t * ev, ti_thing_t * thing, vec_t ** pkgs);
void ti_event_missing_event(uint64_t event_id);
int ti_event_run(ti_event_t * ev);
static inline void ti_event_log(
        const char * prefix,
        ti_event_t * ev,
        int log_level);
void ti__event_log_(const char * prefix, ti_event_t * ev, int log_level);
const char * ti_event_status_str(ti_event_t * ev);

static inline void ti_event_log(
        const char * prefix,
        ti_event_t * ev,
        int log_level)
{
    if (Logger.level <= log_level)
        ti__event_log_(prefix, ev, log_level);
}

#endif /* TI_EVENT_H_ */
