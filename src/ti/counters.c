/*
 * ti/counters.c
 */
#include <assert.h>
#include <ti.h>
#include <ti/counters.h>
#include <util/logger.h>
#include <util/util.h>

static ti_counters_t * counters;
ti_counters_t counters_;

int ti_counters_create(void)
{
    counters = &counters_;

    ti_counters_reset();
    ti.counters = counters;

    return 0;
}

void ti_counters_destroy(void)
{
    counters = ti.counters = NULL;
}

void ti_counters_reset(void)
{
    counters->started_at = util_now_usec();
    counters->queries_success = 0;
    counters->queries_with_error = 0;
    counters->timers_success = 0;
    counters->timers_with_error = 0;
    counters->watcher_failed = 0;
    counters->events_with_gap = 0;
    counters->events_skipped = 0;
    counters->events_failed = 0;
    counters->events_killed = 0;
    counters->events_committed = 0;
    counters->events_quorum_lost = 0;
    counters->events_unaligned = 0;
    counters->largest_result_size = 0;
    counters->queries_from_cache = 0;
    ti_counters_zero_garbage_collected();
    ti_counters_zero_wasted_cache();
    counters->longest_query_duration = 0.0;
    counters->longest_event_duration = 0.0;
    counters->total_query_duration = 0.0;
    counters->total_event_duration = 0.0;
}

/*
 * Returns the duration
 */
double ti_counters_upd_commit_event(struct timespec * start)
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
    return duration;
}

/*
 * Returns the duration
 */
double ti_counters_upd_success_query(struct timespec * start)
{
    struct timespec timing;
    double duration;

    (void) clock_gettime(TI_CLOCK_MONOTONIC, &timing);

    duration = util_time_diff(start, &timing);

    assert (duration > 0);

    ++counters->queries_success;

    if (duration > counters->longest_query_duration)
        counters->longest_query_duration = duration;

    counters->total_query_duration += duration;
    return duration;
}

int ti_counters_to_pk(msgpack_packer * pk)
{
    return -(
        msgpack_pack_map(pk, 21) ||

        mp_pack_str(pk, "queries_success") ||
        msgpack_pack_uint64(pk, counters->queries_success) ||

        mp_pack_str(pk, "queries_with_error") ||
        msgpack_pack_uint64(pk, counters->queries_with_error) ||

        mp_pack_str(pk, "timers_success") ||
        msgpack_pack_uint64(pk, counters->timers_success) ||

        mp_pack_str(pk, "timers_with_error") ||
        msgpack_pack_uint64(pk, counters->timers_with_error) ||

        mp_pack_str(pk, "watcher_failed") ||
        msgpack_pack_uint16(pk, counters->watcher_failed) ||

        mp_pack_str(pk, "events_with_gap") ||
        msgpack_pack_uint64(pk, counters->events_with_gap) ||

        mp_pack_str(pk, "events_skipped") ||
        msgpack_pack_uint64(pk, counters->events_skipped) ||

        mp_pack_str(pk, "events_failed") ||
        msgpack_pack_uint64(pk, counters->events_failed) ||

        mp_pack_str(pk, "events_killed") ||
        msgpack_pack_uint64(pk, counters->events_killed) ||

        mp_pack_str(pk, "events_committed") ||
        msgpack_pack_uint64(pk, counters->events_committed) ||

        mp_pack_str(pk, "events_quorum_lost") ||
        msgpack_pack_uint64(pk, counters->events_quorum_lost) ||

        mp_pack_str(pk, "events_unaligned") ||
        msgpack_pack_uint64(pk, counters->events_unaligned) ||

        mp_pack_str(pk, "garbage_collected") ||
        msgpack_pack_uint64(pk, ti_counters_garbage_collected()) ||

        mp_pack_str(pk, "queries_from_cache") ||
        msgpack_pack_uint64(pk, counters->queries_from_cache) ||

        mp_pack_str(pk, "wasted_cache") ||
        msgpack_pack_uint64(pk, ti_counters_wasted_cache()) ||

        mp_pack_str(pk, "longest_query_duration") ||
        msgpack_pack_double(pk, counters->longest_query_duration) ||

        mp_pack_str(pk, "average_query_duration") ||
        msgpack_pack_double(pk, counters->queries_success
            ? counters->total_query_duration / counters->queries_success
            : 0.0) ||

        mp_pack_str(pk, "longest_event_duration") ||
        msgpack_pack_double(pk, counters->longest_event_duration) ||

        mp_pack_str(pk, "average_event_duration") ||
        msgpack_pack_double(pk, counters->events_committed
            ? counters->total_event_duration / counters->events_committed
            : 0.0) ||

        mp_pack_str(pk, "started_at") ||
        msgpack_pack_uint64(pk, counters->started_at) ||

        mp_pack_str(pk, "largest_result_size") ||
        msgpack_pack_uint64(pk, counters->largest_result_size)
    );
}

ti_val_t * ti_counters_as_mpval(void)
{
    ti_raw_t * raw;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_raw_t), sizeof(ti_raw_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (ti_counters_to_pk(&pk))
    {
        msgpack_sbuffer_destroy(&buffer);
        return NULL;
    }

    raw = (ti_raw_t *) buffer.data;
    ti_raw_init(raw, TI_VAL_MPDATA, buffer.size);

    return (ti_val_t *) raw;
}
