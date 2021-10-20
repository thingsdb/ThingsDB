/*
 * ti/query.t.h
 */
#ifndef TI_QUERY_T_H_
#define TI_QUERY_T_H_

typedef struct ti_query_s ti_query_t;
typedef int (*ti_query_vars_walk_cb)(void * data, void * arg);

#include <cleri/cleri.h>
#include <ti/api.t.h>
#include <ti/closure.t.h>
#include <ti/collection.t.h>
#include <ti/change.t.h>
#include <ti/future.t.h>
#include <ti/qbind.t.h>
#include <ti/stream.t.h>
#include <ti/vtask.t.h>
#include <ti/user.t.h>
#include <ti/val.t.h>
#include <util/util.h>
#include <util/link.h>

enum
{
    TI_QUERY_FLAG_API               =1<<0,
    TI_QUERY_FLAG_CACHE             =1<<1,  /* Queries which are handled by the
                                               query change will have this
                                               flags. Also the first query,
                                               which has not yet a cache item.
                                            */
    TI_QUERY_FLAG_DO_CACHE          =1<<2,  /* mark the query for caching */
    TI_QUERY_FLAG_RAISE_ERR         =1<<3,  /* query->rval contains an error
                                               from a future which must raised
                                               once all futures are done */
    TI_QUERY_FLAG_TASK_CHANGES      =1<<4,  /* mark when this query has handled
                                               all required task changes */
};

typedef enum
{
    TI_QUERY_WITH_PARSERES,
    TI_QUERY_WITH_PROCEDURE,
    TI_QUERY_WITH_FUTURE,
    TI_QUERY_WITH_TASK,
    TI_QUERY_WITH_TASK_FINISH,
} ti_query_with_enum;

typedef int (*ti_query_unpack_cb) (
        ti_query_t *,
        uint16_t,
        const unsigned char *,
        size_t,
        ex_t *);

typedef void (*ti_query_done_cb) (ti_query_t *, ex_t *);
typedef void (*ti_query_run_cb) (ti_query_t *);

typedef union
{
    ti_stream_t * stream;               /* with reference */
    ti_api_request_t * api_request;     /* with ownership of the api_request */
} ti_query_via_t;

typedef union
{
    cleri_parse_t * parseres;   /* parse result */
    ti_closure_t * closure;     /* when called as procedure */
    ti_future_t * future;       /* when called as future->then */
    ti_vtask_t * vtask;         /* when called as task */
} ti_query_with_t;


struct ti_query_s
{
    uint32_t local_stack;       /* variable scopes start here */
    uint16_t pkg_id;            /* package id to return the query to */
    uint8_t with_tp;
    uint8_t flags;
    ti_qbind_t qbind;               /* query binding */
    ti_val_t * rval;                /* return value of a statement */
    ti_collection_t * collection;   /* with reference, NULL when the scope is
                                     * @node or @thingsdb
                                     */
    ti_query_with_t with;
    ti_query_via_t via;         /* with reference */
    ti_user_t * user;           /* with reference, required in case stream
                                   is a node stream */
    vec_t * vars;               /* ti_prop_t - variable */
    ti_change_t * change;       /* with reference, only when a change is
                                   required
                                */
    vec_t * immutable_cache;    /* ti_val_t, Only for immutable and collection
                                   independent variable and temporary used for
                                   procedures to populate the closure arguments
                                */
    link_t futures;             /* place to store futures */
    size_t fcount;              /* future count */
    util_time_t time;           /* time query duration */
};

#endif /* TI_QUERY_T_H_ */
