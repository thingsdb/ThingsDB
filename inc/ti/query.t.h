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
#include <ti/event.t.h>
#include <ti/qbind.t.h>
#include <ti/stream.t.h>
#include <ti/user.t.h>
#include <ti/val.t.h>

typedef int (*ti_query_unpack_cb) (
        ti_query_t *,
        uint16_t,
        const unsigned char *,
        size_t,
        ex_t *);

typedef union
{
    ti_stream_t * stream;               /* with reference */
    ti_api_request_t * api_request;     /* with ownership of the api_request */
} ti_query_via_t;

struct ti_query_s
{
    uint32_t local_stack;       /* variable scopes start here */
    uint16_t pkg_id;            /* package id to return the query to */
    uint16_t pad0;
    ti_qbind_t qbind;               /* query binding */
    ti_val_t * rval;                /* return value of a statement */
    ti_collection_t * collection;   /* with reference, NULL when the scope is
                                     * @node or @thingsdb
                                     */
    char * querystr;            /* 0 terminated query string */

    cleri_parse_t * parseres;   /* parse result */
    ti_closure_t * closure;     /* when called as procedure */
    ti_query_via_t via;         /* with reference */
    ti_user_t * user;           /* with reference, required in case stream
                                   is a node stream */
    vec_t * vars;               /* ti_prop_t - variable */
    ti_event_t * ev;            /* with reference, only when an event is
                                   required
                                */
    vec_t * val_cache;          /* ti_val_t, for node and argument cleanup */
    util_time_t time;           /* time query duration */
};

#endif /* TI_QUERY_T_H_ */
