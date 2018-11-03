/*
 * task.h
 */
#ifndef TI_TASK_H_
#define TI_TASK_H_

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
#include <ti/thing.h>
#include <ti/name.h>
#include <ti/val.h>
#include <util/vec.h>

ti_task_t * ti_task_create(uint64_t event_id, ti_thing_t * thing);
void ti_task_destroy(ti_task_t * task);
int ti_task_add_assign(ti_task_t * task, ti_name_t * name, ti_val_t * val);

struct ti_task_s
{
    uint64_t event_id;
    ti_thing_t * thing;
    vec_t * subtasks;   /* q-pack (unsigned char *) */
};

#endif /* TI_TASK_H_ */
