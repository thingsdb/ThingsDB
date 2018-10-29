/*
 * ti/query.h
 */
#ifndef TI_QUERY_H_
#define TI_QUERY_H_

enum
{
    TI_QUERY_FLAG_WILL_UPDATE=1<<0
};

typedef struct ti_query_s ti_query_t;

#include <ti/raw.h>
#include <ti/db.h>
#include <ti/ex.h>
#include <ti/stream.h>
#include <ti/pkg.h>
#include <cleri/cleri.h>

ti_query_t * ti_query_create(ti_stream_t * stream, ti_pkg_t * pkg);
void ti_query_destroy(ti_query_t * query);
int ti_query_unpack(ti_query_t * query, ex_t * e);
int ti_query_parse(ti_query_t * query, ex_t * e);
int ti_query_investigate(ti_query_t * query, ex_t * e);
void ti_query_run(ti_query_t * query);
void ti_query_send(ti_query_t * query, ex_t * e);
static inline _Bool ti_query_will_update(ti_query_t * query);

struct ti_query_s
{
    uint8_t flags;
    uint64_t pkg_id;
    ti_raw_t * raw;
    ti_db_t * target;   /* target NULL means root */
    char * querystr;
    cleri_parse_t * parseres;
    ti_stream_t * stream;
    vec_t * res_statements;     /* ti_res_t for each statement */
};

static inline _Bool ti_query_will_update(ti_query_t * query)
{
    return query->flags & TI_QUERY_FLAG_WILL_UPDATE;
}

#endif /* TI_QUERY_H_ */
