/*
 * quota.h
 */
#ifndef TI_LIMITS_H_
#define TI_LIMITS_H_

#include <stddef.h>
#include <stdlib.h>

typedef struct ti_quota_s ti_quota_t;


struct ti_quota_s
{
    size_t max_things;
    size_t max_props;           /* max props per thing */
    size_t max_array_size;
    size_t max_raw_size;
};

ti_quota_t * ti_quota_create(void);
inline static void ti_quota_destroy(ti_quota_t * quota);


inline static void ti_quota_destroy(ti_quota_t * quota)
{
    free(quota);
}

#endif  /* TI_LIMITS_H_ */
