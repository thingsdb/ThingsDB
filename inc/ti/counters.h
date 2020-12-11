/*
 * ti/counters.h
 */
#ifndef TI_COUNTERS_H_
#define TI_COUNTERS_H_

typedef struct ti_counters_s ti_counters_t;

#include <inttypes.h>
#include <sys/time.h>
#include <ti/val.h>
#include <util/mpack.h>

int ti_counters_create(void);
void ti_counters_destroy(void);
void ti_counters_reset(void);
double ti_counters_upd_commit_event(struct timespec * start);
double ti_counters_upd_success_query(struct timespec * start);
int ti_counters_to_pk(msgpack_packer * pk);
ti_val_t * ti_counters_as_mpval(void);

struct ti_counters_s
{
    uint64_t started_at;            /* time when counters were reset */
    uint64_t queries_success;       /* node queries where this node acted as
                                       the master node and the query was
                                       successful finished
                                    */
    uint64_t queries_with_error;    /* node queries where this node acted as
                                       the master node but the query has
                                       returned with an error
                                    */
    uint16_t watcher_failed;        /* failed to update a watcher with an
                                       event
                                    */
    uint64_t events_with_gap;       /* events which are committed but at least
                                       one event id was skipped
                                    */
    uint64_t events_skipped;        /* events which cannot be committed because
                                       an event with a higher id is already
                                       been committed
                                    */
    uint64_t events_failed;         /* events failed which should be committed
                                       from EPKG, this is a critical counter
                                       since the EPKG events should proceed
                                       without errors
                                    */
    uint64_t events_killed;         /* event killed because it took too long
                                       before the event got the READY status.
                                       these events may later be received
                                       and still being committed or skipped
                                    */
    uint64_t events_committed;      /* events committed */
    uint64_t events_quorum_lost;    /* number of times a quorum was not reached
                                       for requesting an event id
                                    */
    uint64_t events_unaligned;      /* number of times an event cannot be
                                       pushed to the end of the queue because
                                       a higher event id is already queued
                                    */
    uint64_t garbage_collected;     /* total garbage collected */
    uint64_t largest_result_size;   /* largest result size in bytes */
    uint64_t queries_from_cache;    /* number of queries which are loaded from
                                       cache.
                                    */
    uint64_t dropped_query_cache;   /* number of cached queries which are
                                       removed from the cache before the cache
                                       was ever used. Basically this count the
                                       useless caching.
                                    */
    double longest_query_duration;  /* longest duration it took for a
                                       successful query to process (in seconds)
                                    */
    double longest_event_duration;  /* longest duration it took for an event
                                       to complete (in seconds)
                                    */
    double total_query_duration;    /* can be used to calculate the average
                                       total_query_duration / queries_success
                                        (in seconds)
                                    */
    double total_event_duration;    /* can be used to calculate the average
                                       total_event_duration / events_committed
                                        (in seconds)
                                    */
};

#endif  /* TI_COUNTERS_H_ */
