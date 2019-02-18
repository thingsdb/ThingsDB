/*
 * util/query.h
 */
#ifndef QUERY_H_
#define QUERY_H_

#include <assert.h>
#include <ti/event.h>
#include <ti/task.h>
#include <ti/val.h>
#include <ti/name.h>
#include <ti/scope.h>
#include <ti/query.h>
#include <ti/prop.h>
#include <ti/ex.h>
#include <ti/thing.h>
#include <util/vec.h>

static inline const char * query_tp_str(ti_query_t * query);
static inline _Bool query_is_thing(ti_query_t * query);
static inline _Bool query_in_use_thing(ti_query_t * query);
static inline _Bool query_is_raw(ti_query_t * query);
static inline ti_thing_t * query_weak_get_thing(ti_query_t * query);
ti_prop_t * query_get_tmp_prop(ti_query_t * query, ti_name_t * name);
static inline ti_val_t * query_get_val(ti_query_t * query);


static inline const char * query_tp_str(ti_query_t * query)
{
    return query->rval
            ? ti_val_str(query->rval)
            : (query->scope->val
                    ? ti_val_str(query->scope->val)
                    : TI_VAL_THING_S
            );
}

static inline ti_val_t * query_weak_get_val(ti_query_t * query)
{
    return query->rval ? query->rval : ti_scope_weak_get_val(query->scope);
}

static inline _Bool query_is_thing(ti_query_t * query)
{
    return (
        (query->rval && query->rval->tp == TI_VAL_THING) ||
        (!query->rval && ti_scope_is_thing(query->scope))
    );
}

static inline _Bool query_in_use_thing(ti_query_t * query)
{
    assert (query_is_thing(query));
    return query->rval
            ? ti_scope_in_use_thing(query->scope, query->rval)
            : ti_scope_in_use_thing(query->scope->prev, query->scope->thing);
}

static inline _Bool query_is_raw(ti_query_t * query)
{
    return (
        (query->rval && ti_val_is_raw(query->rval)) ||
        (!query->rval && ti_scope_is_raw(query->scope))
    );
}

/* returns a borrowed reference */
static inline ti_thing_t * query_weak_get_thing(ti_query_t * query)
{
    return query->rval ? query->rval : query->scope->thing;
}

/* return value might be NULL */
static inline ti_val_t * query_get_val(ti_query_t * query)
{
    return query->rval ? query->rval : query->scope->val;
}

static inline _Bool query_is_prop(ti_query_t * query)
{
    return !query->rval;
}


#endif  /* QUERY_H_ */
