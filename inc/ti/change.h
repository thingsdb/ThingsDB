/*
 * ti/change.h
 */
#ifndef TI_CHANGE_H_
#define TI_CHANGE_H_

#include <stdint.h>
#include <sys/time.h>
#include <ti/change.t.h>
#include <ti/thing.t.h>
#include <util/vec.h>
#include <util/logger.h>

ti_change_t * ti_change_create(ti_change_tp_enum tp);
ti_change_t * ti_change_cpkg(ti_cpkg_t * cpkg);
ti_change_t * ti_change_initial(void);
void ti_change_drop(ti_change_t * change);
void ti_change_missing(uint64_t change_id);
int ti_change_run(ti_change_t * change);
void ti__change_log_(const char * prefix, ti_change_t * change, int log_level);
const char * ti_change_status_str(ti_change_t * change);

static inline void ti_change_log(
        const char * prefix,
        ti_change_t * change,
        int log_level)
{
    if (Logger.level <= log_level)
        ti__change_log_(prefix, change, log_level);
}

#endif /* TI_CHANGE_H_ */
