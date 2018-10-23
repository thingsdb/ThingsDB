/*
 * limits.c
 */
#include <stdint.h>
#include <ti/limits.h>



ti_limits_t * ti_limits_create(void)
{
    ti_limits_t * limits = malloc(sizeof(ti_limits_t));
    if (!limits)
        return NULL;

    limits->max_things = SIZE_MAX;
    limits->max_props = SIZE_MAX;
    limits->max_array_size = SIZE_MAX;
    limits->max_raw_size = SIZE_MAX;

    return limits;
}

