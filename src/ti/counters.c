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
    (void) clock_gettime(TI_CLOCK_MONOTONIC, &counters->reset_time);
    counters->queries_received = 0;
    counters->events_with_gap = 0;
    counters->events_skipped = 0;
    counters->events_failed = 0;
    counters->events_killed = 0;
    counters->events_committed = 0;
    counters->events_quorum_lost = 0;
    counters->events_unaligned = 0;
    counters->garbage_collected = 0;
    counters->longest_event_duration = 0.0f;
    counters->total_event_duration = 0.0f;
}

void ti_counters_upd_commit_event(struct timespec * start)
{
    struct timespec timing;
    double duration;

    (void) clock_gettime(TI_CLOCK_MONOTONIC, &timing);

    duration = util_time_diff(start, &timing);

    assert (duration > 0);

    ++counters->events_committed;

    if (duration > counters->longest_event_duration)
        counters->longest_event_duration = duration;

    counters->total_event_duration += duration;
}

int ti_counters_to_packer(qp_packer_t ** packer)
{
    return -(
        qp_add_map(packer) ||
        qp_add_raw_from_str(*packer, "queries_received") ||
        qp_add_int(*packer, counters->queries_received) ||
        qp_add_raw_from_str(*packer, "events_with_gap") ||
        qp_add_int(*packer, counters->events_with_gap) ||
        qp_add_raw_from_str(*packer, "events_skipped") ||
        qp_add_int(*packer, counters->events_skipped) ||
        qp_add_raw_from_str(*packer, "events_failed") ||
        qp_add_int(*packer, counters->events_failed) ||
        qp_add_raw_from_str(*packer, "events_killed") ||
        qp_add_int(*packer, counters->events_killed) ||
        qp_add_raw_from_str(*packer, "events_committed") ||
        qp_add_int(*packer, counters->events_committed) ||
        qp_add_raw_from_str(*packer, "events_quorum_lost") ||
        qp_add_int(*packer, counters->events_quorum_lost) ||
        qp_add_raw_from_str(*packer, "events_unaligned") ||
        qp_add_int(*packer, counters->events_unaligned) ||
        qp_add_raw_from_str(*packer, "garbage_collected") ||
        qp_add_int(*packer, counters->garbage_collected) ||
        qp_add_raw_from_str(*packer, "longest_event_duration") ||
        qp_add_double(*packer, counters->longest_event_duration) ||
        qp_add_raw_from_str(*packer, "average_event_duration") ||
        qp_add_double(*packer, counters->events_committed
            ? counters->total_event_duration / counters->events_committed
            : 0.0f) ||
        qp_close_map(*packer)
    );
}

ti_val_t * ti_counters_as_qpval(void)
{
    ti_raw_t * raw;
    ti_val_t * qpval = NULL;
    qp_packer_t * packer = qp_packer_create2(512, 1);
    if (!packer)
        return NULL;

    if (ti_counters_to_packer(&packer))
        goto fail;

    raw = ti_raw_from_packer(packer);
    if (!raw)
        goto fail;

    qpval = ti_val_weak_create(TI_VAL_QP, raw);
    if (!qpval)
        ti_raw_drop(raw);

fail:
    qp_packer_destroy(packer);
    return qpval;
}
