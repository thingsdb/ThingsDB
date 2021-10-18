/*
 * ti/tasks.h
 */
#ifndef TI_TASKS_H_
#define TI_TASKS_H_

typedef struct ti_tasks_s ti_tasks_t;

/* minimal repeat value of 60 seconds (one minute) */
#define TI_TASKS_MIN_REPEAT 60

#include <ti/vtask.t.h>
#include <ti/user.t.h>
#include <ti/varr.t.h>
#include <util/vec.h>
#include <uv.h>


int ti_tasks_create(void);
int ti_tasks_start(void);
void ti_tasks_stop(void);
void ti_tasks_clear_dropped(vec_t ** vtasks);
void ti_tasks_del_user(ti_user_t * user);
ti_varr_t * ti_tasks_list(vec_t * tasks);
vec_t * ti_tasks_from_scope_id(uint64_t scope_id);

struct ti_tasks_s
{
    _Bool is_stopping;
    _Bool is_started;
    vec_t * vtasks;         /* ti_vtask_t */
    uv_timer_t * timer;
};

#endif  /* TI_TASKS_H_ */
