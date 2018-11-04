/*
 * util/res.h
 */
#ifndef RES_H_
#define RES_H_

#include <ti/event.h>
#include <ti/thing.h>
#include <ti/task.h>
#include <ti/ex.h>
#include <util/vec.h>

void res_destroy_collect_cb(vec_t * names);
ti_task_t * res_get_task(ti_event_t * ev, ti_thing_t * thing, ex_t * e);


#endif  /* RES_H_ */
