/*
 * limits.h
 */
#ifndef TI_LIMITS_H_
#define TI_LIMITS_H_

#include <stddef.h>
#include <stdlib.h>

typedef struct ti_limits_s ti_limits_t;


struct ti_limits_s
{
    size_t max_things;
    size_t max_props;           /* max props per thing */
    size_t max_array_size;
    size_t max_raw_size;
};

ti_limits_t * ti_limits_create(void);
inline static void ti_limits_destroy(ti_limits_t * limits);


inline static void ti_limits_destroy(ti_limits_t * limits)
{
    free(limits);
}

#endif  /* TI_LIMITS_H_ */
