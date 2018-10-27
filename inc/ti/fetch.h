/*
 * ti/fetch.h
 */

typedef struct ti_fetch_s ti_fetch_t;

#include <ti/thing.h>
#include <util/vec.h>

struct ti_fetch_s
{
    ti_thing_t * thing;
    vec_t * names;
};
