/*
 * query.h
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
#include <cleri/cleri.h>

ti_query_t * ti_query_create(const unsigned char * data, size_t n);
void ti_query_destroy(ti_query_t * query);
int ti_query_unpack(ti_query_t * query, ex_t * e);
int ti_query_parse(ti_query_t * query, ex_t * e);
int ti_query_investigate(ti_query_t * query, ex_t * e);
static inline _Bool ti_query_will_update(ti_query_t * query);

struct ti_query_s
{
    uint8_t flags;
    ti_raw_t * raw;
    ti_db_t * target;   /* target NULL means root */
    char * querystr;
    cleri_parse_t * parseres;
};

static inline _Bool ti_query_will_update(ti_query_t * query)
{
    return query->flags & TI_QUERY_FLAG_WILL_UPDATE;
}

#endif /* TI_QUERY_H_ */
