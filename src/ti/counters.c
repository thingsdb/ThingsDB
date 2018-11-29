/*
 * ti/counters.c
 */
#include <assert.h>
#include <ti.h>
#include <ti/counters.h>
#include <util/logger.h>
#include <util/util.h>

static ti_counters_t * counters;

int ti_counters_create(void)
{
    counters = malloc(sizeof(ti_counters_t));
    if (!counters)
        return -1;

    ti_counters_reset();

    ti()->counters = counters;

    return 0;
}

void ti_counters_destroy(void)
{
    free(counters);
    counters = ti()->counters = NULL;
}

void ti_counters_reset(void)
{
    counters->events_out_of_order = 0;
    counters->events_killed = 0;
    counters->events_committed = 0;
    counters->events_failed = 0;
    counters->garbage_collected = 0;
    counters->longest_event_duration = 0.0f;
    counters->total_event_duration = 0.0f;
}

void ti_counters_upd_commit_event(struct timespec * start)
{
    struct timespec timing;
    double duration;

    if (clock_gettime(TI_CLOCK_MONOTONIC, &timing))
        return;

    duration = util_time_diff(start, &timing);

    assert (duration > 0);

    ++counters->events_committed;

    if (duration > counters->longest_event_duration)
        counters->longest_event_duration = duration;

    counters->total_event_duration += duration;
}
