/*
 * ti/counters.h
 */
#ifndef TI_COUNTERS_H_
#define TI_COUNTERS_H_

typedef struct ti_counters_s ti_counters_t;

#include <inttypes.h>
#include <sys/time.h>

int ti_counters_create(void);
void ti_counters_destroy(void);
void ti_counters_reset(void);
void ti_counters_upd_commit_event(struct timespec * start);

struct ti_counters_s
{
    uint64_t queries_received;      /* queries where this node acted as the
                                       master node
                                    */
    uint64_t events_with_gap;       /* events which are committed but at least
                                       one event id was skipped
                                    */
    uint64_t events_skipped;        /* events which cannot be committed because
                                       an event with a higher id is already
                                       been committed, these events are moved
                                       to a `skipped` queue
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
    double longest_event_duration;  /* longest duration it took for an event
                                       to complete
                                    */
    double total_event_duration;    /* can be used to calculate the average
                                       total_event_duration / events_committed
                                    */
};

#endif  /* TI_COUNTERS_H_ */
