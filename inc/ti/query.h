/*
 * ti/query.h
 */
#ifndef TI_QUERY_H_
#define TI_QUERY_H_

typedef struct ti_query_s ti_query_t;

#include <cleri/cleri.h>
#include <ti/collection.h>
#include <ti/event.h>
#include <ex.h>
#include <ti/name.h>
#include <ti/pkg.h>
#include <ti/prop.h>
#include <ti/raw.h>
#include <ti/user.h>
#include <ti/qbind.h>
#include <ti/stream.h>
#include <ti/api.h>
#include <ti/scope.h>
#include <ti/closure.h>
#include <util/omap.h>

typedef int (*ti_query_unpack_cb) (
        ti_query_t *,
        uint16_t,
        const uchar *,
        size_t,
        ex_t *);

ti_query_t * ti_query_create(void * via, ti_user_t * user, uint8_t flags);
void ti_query_destroy(ti_query_t * query);
int ti_query_unpack(
        ti_query_t * query,
        ti_scope_t * scope,
        uint16_t pkg_id,
        const unsigned char * data,
        size_t n,
        ex_t * e);
int ti_query_unp_run(
        ti_query_t * query,
        ti_scope_t * scope,
        uint16_t pkg_id,
        const unsigned char * data,
        size_t n,
        ex_t * e);
int ti_query_parse(ti_query_t * query, ex_t * e);
int ti_query_investigate(ti_query_t * query, ex_t * e);
void ti_query_run(ti_query_t * query);
void ti_query_send_response(ti_query_t * query, ex_t * e);
int ti_query_unpack_args(ti_query_t * query, ti_vup_t * vup, ex_t * e);
int ti_query_apply_scope(ti_query_t * query, ti_scope_t * scope, ex_t * e);
int ti_query_val_gc(ti_val_t * val, ti_query_t * query);
ti_prop_t * ti_query_var_get(ti_query_t * query, ti_name_t * name);
ti_thing_t * ti_query_thing_from_id(
        ti_query_t * query,
        int64_t thing_id,
        ex_t * e);
size_t ti_query_count_type(ti_query_t * query, ti_type_t * type);
static inline _Bool ti_query_will_update(ti_query_t * query);
static inline const char * ti_query_scope_name(ti_query_t * query);

typedef union
{
    ti_stream_t * stream;               /* with reference */
    ti_api_request_t * api_request;     /* with ownership of the api_request */
} ti_query_via_t;

struct ti_query_s
{
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
    vec_t * gc;                 /* ti_vat_t, requires garbage collection
                                   only wrapped values and things will be added
                                   since they are by reference */
    util_time_t time;           /* time query duration */
};

static inline _Bool ti_query_will_update(ti_query_t * query)
{
    return query->qbind.flags & TI_QBIND_FLAG_EVENT;
}

static inline const char * ti_query_scope_name(ti_query_t * query)
{
    return query->qbind.flags & TI_QBIND_FLAG_NODE
            ? "@node"
            : query->qbind.flags & TI_QBIND_FLAG_THINGSDB
            ? "@thingsdb"
            : query->qbind.flags & TI_QBIND_FLAG_COLLECTION
            ? "@collection"
            : "<unknown>";
}
#endif /* TI_QUERY_H_ */
