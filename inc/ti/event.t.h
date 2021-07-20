/*
 * ti/event.t.h
 */
#ifndef TI_EVENT_T_H_
#define TI_EVENT_T_H_

typedef enum
{
    TI_EVENT_STAT_NEW,      /* as long as it is not accepted by all */
    TI_EVENT_STAT_CACNCEL,  /* event is cancelled due to an error */
    TI_EVENT_STAT_READY,    /* all nodes accepted the id */
} ti_event_status_enum;

typedef enum
{
    TI_EVENT_TP_MASTER,     /* status can be anything */
    TI_EVENT_TP_EPKG,       /* status is always READY */
} ti_event_tp_enum;

typedef enum
{
    TI_EVENT_FLAG_SAVE      = 1<<0,    /* ti_save() must be triggered */
} ti_event_flags_enum;

typedef struct ti_event_s ti_event_t;
typedef union ti_event_u ti_event_via_t;

#include <inttypes.h>
#include <ti/collection.t.h>
#include <ti/epkg.t.h>
#include <ti/query.t.h>
#include <util/util.h>
#include <util/vec.h>

union ti_event_u
{
    ti_query_t * query;         /* TI_EVENT_TP_MASTER (no reference) */
    ti_epkg_t * epkg;           /* TI_EVENT_TP_EPKG (with reference) */
};

struct ti_event_s
{
    uint32_t ref;           /* reference counting */
    uint8_t status;         /* NEW / CANCEL / READY */
    uint8_t tp;             /* MASTER / EPKG */
    uint8_t flags;
    uint8_t requests;       /* when master, keep the `n` requests */
    uint64_t id;
    ti_event_via_t via;
    ti_collection_t * collection;   /* collection with reference or NULL */
    vec_t * _tasks;                 /* ti_task_t */
    util_time_t time;               /* timing an event, used for elapsed
                                     * time etc.
                                     */
};

#endif /* TI_EVENT_T_H_ */
