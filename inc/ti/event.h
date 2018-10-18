/*
 * event.h
 */
#ifndef TI_EVENT_H_
#define TI_EVENT_H_

enum
{
    TI_EVENT_STAT_UNINITIALIZED,
    TI_EVENT_STAT_REG, /* as long as it is not accepted by all */
    TI_EVENT_STAT_CACNCEL,  /* only in case of an error */
    TI_EVENT_STAT_READY, /* all nodes accept the id */
};

enum
{
    TI_EVENT_FLAG
};

typedef struct ti_event_s ti_event_t;

#include <stdint.h>
#include <qpack.h>
#include <ti/db.h>
#include <events.h>
#include <ti/pkg.h>
#include <ti/prom.h>
#include <ti/raw.h>
#include <ti/sock.h>
#include <util/imap.h>
#include <util/vec.h>

ti_event_t * ti_event_create(void);
void ti_event_destroy(ti_event_t * event);
void ti_event_new(ti_sock_t * sock, ti_pkg_t * pkg, ex_t * e);
int ti_event_init(ti_event_t * event);
void ti_event_raw(
        ti_event_t * event,
        const unsigned char * raw,
        size_t sz,
        ex_t * e);
int ti_event_run(ti_event_t * event);
void ti_event_finish(ti_event_t * event);


struct ti_event_s
{
    uint64_t id;
    uint16_t pid;
    uint8_t status;
    uint8_t flags;
    ti_db_t * target;       // NULL for _tin or pointer to database
    ti_node_t * node;
    ti_sock_t * client;     // NULL or requesting client
    ti_raw_t * raw;
    imap_t * refthings;
    vec_t * tasks;          /* each task is a qp_res_t */
    vec_t * nodes;          /* task is registered on these nodes */
    qp_packer_t * result;
    ti_prom_t * prom;
    ti_pkg_t * req_pkg;     /* used as temporary package pointer */
};

#endif /* TI_EVENT_H_ */
