/*
 * ti/task.t.h
 */
#ifndef TI_TASK_T_H_
#define TI_TASK_T_H_

typedef enum
{
    TI_TASK_ASSIGN,
    TI_TASK_DEL,
    TI_TASK_PUSH,
    TI_TASK_SET,
    TI_TASK_UNSET,
} ti_task_enum;

typedef struct ti_task_s ti_task_t;

#include <inttypes.h>
#include <stdlib.h>
#include <ti/thing.t.h>
#include <util/vec.h>

struct ti_task_s
{
    uint64_t event_id;
    size_t approx_sz;
    ti_thing_t * thing;     /* with reference */
    vec_t * jobs;           /* q-pack (unsigned char *) */
};

#endif /* TI_TASK_T_H_ */
