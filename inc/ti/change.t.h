/*
 * ti/change.t.h
 */
#ifndef TI_CHANGE_T_H_
#define TI_CHANGE_T_H_

typedef enum
{
    TI_CHANGE_STAT_NEW,     /* as long as it is not accepted by all */
    TI_CHANGE_STAT_CACNCEL, /* change is cancelled due to an error */
    TI_CHANGE_STAT_READY,   /* all nodes accepted the id */
} ti_change_status_enum;

typedef enum
{
    TI_CHANGE_TP_MASTER,     /* status can be anything */
    TI_CHANGE_TP_CPKG,       /* status is always READY */
} ti_change_tp_enum;

typedef enum
{
    TI_CHANGE_FLAG_SAVE      = 1<<0,    /* ti_save() must be triggered */
} ti_change_flags_enum;

typedef struct ti_change_s ti_change_t;
typedef union ti_change_u ti_change_via_t;

#include <inttypes.h>
#include <ti/collection.t.h>
#include <ti/cpkg.t.h>
#include <ti/query.t.h>
#include <util/util.h>
#include <util/vec.h>

union ti_change_u
{
    ti_query_t * query;         /* TI_CHANGE_TP_MASTER (no reference) */
    ti_cpkg_t * cpkg;           /* TI_CHANGE_TP_CPKG (with reference) */
};

struct ti_change_s
{
    uint32_t ref;           /* reference counting */
    uint8_t status;         /* NEW / CANCEL / READY */
    uint8_t tp;             /* MASTER / CPKG */
    uint8_t flags;
    uint8_t requests;       /* when master, keep the `n` requests */
    uint64_t id;
    ti_change_via_t via;
    ti_collection_t * collection;   /* collection with reference or NULL */
    vec_t * tasks;                 /* ti_task_t */
    util_time_t time;               /* timing a change, used for elapsed
                                     * time etc.
                                     */
};

#endif /* TI_CHANGE_T_H_ */
