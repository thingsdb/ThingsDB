/*
 * util/res.h
 */
#ifndef RES_H_
#define RES_H_

#include <assert.h>
#include <ti/event.h>
#include <ti/thing.h>
#include <ti/task.h>
#include <ti/val.h>
#include <ti/name.h>
#include <ti/scope.h>
#include <ti/res.h>
#include <ti/ex.h>
#include <util/vec.h>

void res_destroy_collect_cb(vec_t * names);
ti_task_t * res_get_task(ti_event_t * ev, ti_thing_t * thing, ex_t * e);
static inline ti_val_enum res_tp(ti_res_t * res);
static inline const char * res_tp_str(ti_res_t * res);
static inline _Bool res_is_thing(ti_res_t * res);
static inline _Bool res_is_raw(ti_res_t * res);
static inline ti_thing_t * res_get_thing(ti_res_t * res);
int res_rval_clear(ti_res_t * res);
void res_rval_destroy(ti_res_t * res);
void res_rval_weak_destroy(ti_res_t * res);
static inline ti_val_t * res_get_val(ti_res_t * res);

static inline ti_val_enum res_tp(ti_res_t * res)
{
    return res->rval ? res->rval->tp : (
            res->scope->val ? res->scope->val->tp : TI_VAL_THING);
}

static inline const char * res_tp_str(ti_res_t * res)
{
    return ti_val_tp_str(res_tp(res));
}

static inline _Bool res_is_thing(ti_res_t * res)
{
    return (
        (res->rval && res->rval->tp == TI_VAL_THING) ||
        (!res->rval && ti_scope_is_thing(res->scope))
    );
}

static inline _Bool res_is_raw(ti_res_t * res)
{
    return (
        (res->rval && ti_val_is_raw(res->rval)) ||
        (!res->rval && ti_scope_is_raw(res->scope))
    );
}

/* returns a borrowed reference */
static inline ti_thing_t * res_get_thing(ti_res_t * res)
{
    assert (res_is_thing(res));
    return res->rval ? res->rval->via.thing : res->scope->thing;
}

/* return value might be NULL */
static inline ti_val_t * res_get_val(ti_res_t * res)
{
    return res->rval ? res->rval : res->scope->val;
}


#endif  /* RES_H_ */
