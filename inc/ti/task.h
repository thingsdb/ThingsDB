/*
 * task.h
 */
#ifndef TI_TASK_H_
#define TI_TASK_H_

#define TI_TASK_N 15

/*
 * Max 60 tasks can be defined.
 */
typedef enum
{
    TI_TASK_USER_CREATE,
    TI_TASK_USER_DROP,
    TI_TASK_USER_ALTER,
    TI_TASK_DB_CREATE,
    TI_TASK_DB_RENAME,
    TI_TASK_DB_DROP,
    TI_TASK_GRANT,
    TI_TASK_REVOKE,
    TI_TASK_SET_REDUNDANCY,
    TI_TASK_NODE_ADD,
    TI_TASK_NODE_REPLACE,
    TI_TASK_SUBSCRIBE,
    TI_TASK_UNSUBSCRIBE,
    TI_TASK_PROPS_SET,
    TI_TASK_PROPS_DEL,
} ti_task_e;

typedef enum
{
    TI_TASK_ERR=-1,
    TI_TASK_SUCCESS,
    TI_TASK_FAILED,
    TI_TASK_SKIPPED
} ti_task_stat_e;

typedef struct ti_task_s ti_task_t;

#include <qpack.h>
#include <ti/event.h>#include <util/ex.h>

ti_task_t * ti_task_create(qp_res_t * res, ex_t * e);
void ti_task_destroy(ti_task_t * task);
ti_task_stat_e ti_task_run(
        ti_task_t * task,
        ti_event_t * event,
        ti_task_stat_e rc);
const char * ti_task_str(ti_task_t * task);

struct ti_task_s
{
    ti_task_e tp;
    qp_res_t * res;
};

#endif /* TI_TASK_H_ */
