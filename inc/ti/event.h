/*
 * event.h
 */
#ifndef TI_EVENT_H_
#define TI_EVENT_H_

typedef enum
{
    TI_EVENT_STAT_NEW,      /* as long as it is not accepted by all */
    TI_EVENT_STAT_CACNCEL,  /* event is cancelled due to an error */
    TI_EVENT_STAT_READY,    /* all nodes accepted the id */
} ti_event_status_enum;

typedef enum
{
    TI_EVENT_TP_MASTER,     /* status can be anything */
    TI_EVENT_TP_SLAVE,      /* status is never READY */
    TI_EVENT_TP_EPKG,       /* status is always READY */
} ti_event_tp_enum;

typedef struct ti_event_s ti_event_t;
typedef union ti_event_u ti_event_via_t;

#include <qpack.h>
#include <stdint.h>
#include <sys/time.h>
#include <ti/collection.h>
#include <ti/events.h>
#include <ti/pkg.h>
#include <ti/prom.h>
#include <ti/query.h>
#include <ti/raw.h>
#include <ti/stream.h>
#include <util/omap.h>

ti_event_t * ti_event_create(ti_event_tp_enum tp);
ti_event_t * ti_event_initial(void);
void ti_event_drop(ti_event_t * ev);
int ti_event_run(ti_event_t * ev);
void ti_event_log(const char * prefix, ti_event_t * ev);
const char * ti_event_status_str(ti_event_t * ev);

union ti_event_u
{
    ti_query_t * query;         /* TI_EVENT_TP_MASTER (no reference) */
    ti_node_t * node;           /* TI_EVENT_TP_SLAVE (with reference) */
    ti_epkg_t * epkg;           /* TI_EVENT_TP_EPKG (with reference) */
};

struct ti_event_s
{
    uint32_t ref;           /* reference counting */
    uint16_t pad0;
    uint8_t status;         /* NEW / CANCEL / READY */
    uint8_t tp;             /* MASTER / SLAVE / EPKG */
    uint64_t id;
    ti_event_via_t via;
    ti_collection_t * target;   /* NULL for root or collection with reference
                                */
    omap_t * tasks;             /* thing_id : ti_task_t */
    struct timespec time;       /* timing an event, used for elapsed time etc.
                                */
};

#endif /* TI_EVENT_H_ */
