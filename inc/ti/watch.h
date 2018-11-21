/*
 * ti/watch.h
 */
#ifndef TI_WATCH_H_
#define TI_WATCH_H_

typedef struct ti_watch_s ti_watch_t;

#include <ti/stream.h>

ti_watch_t * ti_watch_create(uint64_t id, ti_stream_t * stream);
void ti_watch_destroy(ti_watch_t * watch);
void ti_watch_drop_thing(uint64_t id);
void ti_watch_destroy_all(void);

struct ti_watch_s
{
    ti_stream_t * stream;       /* weak reference */
    ti_watch_t * prev;
    ti_watch_t * next;
};

#endif  /* TI_WATCH_H_ */
