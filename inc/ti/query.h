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
#include <ti/syntax.h>
#include <ti/stream.h>
#include <ti/chain.h>
#include <ti/closure.h>
#include <util/omap.h>

typedef int (*ti_query_unpack_cb) (
        ti_query_t *,
        uint16_t,
        const uchar *,
        size_t,
        ex_t *);

ti_query_t * ti_query_create(ti_stream_t * stream, ti_user_t * user);
void ti_query_destroy(ti_query_t * query);
int ti_query_run_unpack(
        ti_query_t * query,
        uint16_t pkg_id,
        const uchar * data,
        size_t n,
        ex_t * e);
int ti_query_node_unpack(
        ti_query_t * query,
        uint16_t pkg_id,
        const uchar * data,
        size_t n,
        ex_t * e);
int ti_query_thingsdb_unpack(
        ti_query_t * query,
        uint16_t pkg_id,
        const uchar * data,
        size_t n,
        ex_t * e);
int ti_query_collection_unpack(
        ti_query_t * query,
        uint16_t pkg_id,
        const uchar * data,
        size_t n,
        ex_t * e);
int ti_query_parse(ti_query_t * query, ex_t * e);
int ti_query_investigate(ti_query_t * query, ex_t * e);
void ti_query_run(ti_query_t * query);
void ti_query_send(ti_query_t * query, ex_t * e);
ti_prop_t * ti_query_var_get(ti_query_t * query, ti_name_t * name);
ti_thing_t * ti_query_thing_from_id(
        ti_query_t * query,
        int64_t thing_id,
        ex_t * e);
static inline _Bool ti_query_will_update(ti_query_t * query);
static inline const char * ti_query_scope_name(ti_query_t * query);

struct ti_query_s
{
    ti_syntax_t syntax;         /* syntax binding */
    ti_chain_t chain;           /* ti_chain_t */
    ti_val_t * rval;            /* return value of a statement */
    ti_collection_t * target;   /* with reference,
                                   NULL when target is node or thingsdb */
    ti_thing_t * root;          /* thing0 or target->root */
    char * querystr;            /* 0 terminated query string */
    cleri_parse_t * parseres;   /* parse result */
    ti_closure_t * closure;     /* when called as procedure */
    ti_stream_t * stream;       /* with reference */
    ti_user_t * user;           /* with reference, required in case stream
                                   is a node stream */
    vec_t * blobs;              /* ti_raw_t */
    vec_t * vars;               /* ti_prop_t - variable */
    ti_event_t * ev;            /* with reference, only when an event is
                                   required
                                */
    vec_t * val_cache;          /* ti_val_t, for node and argument cleanup */
};

static inline _Bool ti_query_will_update(ti_query_t * query)
{
    return query->syntax.flags & TI_SYNTAX_FLAG_EVENT;
}

static inline const char * ti_query_scope_name(ti_query_t * query)
{
    if (query->syntax.flags & TI_SYNTAX_FLAG_NODE)
        return "node";
    else if (query->syntax.flags & TI_SYNTAX_FLAG_THINGSDB)
        return "thingsdb";
    else
        return "collection";
}
#endif /* TI_QUERY_H_ */
