/*
 * event.h
 */
#ifndef TI_EVENT_H_
#define TI_EVENT_H_

enum
{
    TI_EVENT_STAT_NEW,      /* as long as it is not accepted by all */
    TI_EVENT_STAT_CACNCEL,  /* event is cancelled due to an error */
    TI_EVENT_STAT_PREPARE,  /* done on master, created the abstract tasks */
    TI_EVENT_STAT_READY,    /* all nodes accepted the id */
};

typedef enum
{
    TI_EVENT_TP_MASTER,
    TI_EVENT_TP_SLAVE,
} ti_event_tp_enum;

typedef struct ti_event_s ti_event_t;
typedef union ti_event_u ti_event_via_t;

#include <stdint.h>
#include <qpack.h>
#include <ti/db.h>
#include <ti/events.h>
#include <ti/pkg.h>
#include <ti/prom.h>
#include <ti/raw.h>
#include <ti/stream.h>
#include <util/imap.h>
#include <util/vec.h>
#include <ti/query.h>

ti_event_t * ti_event_create(ti_event_tp_enum tp);
void ti_event_destroy(ti_event_t * ev);
void ti_event_cancel(ti_event_t * ev);

union ti_event_u
{
    ti_query_t * query;         /* TI_EVENT_TP_MASTER */
    ti_node_t * node;           /* TI_EVENT_TP_SLAVE */
};

struct ti_event_s
{
    uint64_t id;
    uint8_t status;
    uint8_t tp;             /* master or slave */
    ti_event_via_t via;
    ti_db_t * target;       /* NULL for root or pointer to database */
};

#endif /* TI_EVENT_H_ */
