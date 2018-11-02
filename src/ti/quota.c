/*
 * quota.c
 */
#include <stdint.h>
#include <ti/quota.h>



ti_quota_t * ti_quota_create(void)
{
    ti_quota_t * quota = malloc(sizeof(ti_quota_t));
    if (!quota)
        return NULL;

    quota->max_things = SIZE_MAX;
    quota->max_props = 2;
    quota->max_array_size = SIZE_MAX;
    quota->max_raw_size = SIZE_MAX;

    return quota;
}

