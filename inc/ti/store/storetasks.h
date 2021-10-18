/*
 * ti/store/tasks.h
 */
#ifndef TI_STORE_TASKS_H_
#define TI_STORE_TASKS_H_

#include <util/vec.h>
#include <ti/collection.h>

int ti_store_tasks_store(vec_t * tasks, const char * fn);
int ti_store_tasks_restore(
        vec_t ** tasks,
        const char * fn,
        ti_collection_t * collection);

#endif /* TI_STORE_TASKS_H_ */
