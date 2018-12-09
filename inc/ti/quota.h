/*
 * quota.h
 */
#ifndef TI_QUOTA_H_
#define TI_QUOTA_H_

#include <stddef.h>
#include <stdlib.h>
#include <ti/ex.h>

typedef struct ti_quota_s ti_quota_t;

#define TI_QUOTA_NOT_SET SIZE_MAX

enum
{
    QUOTA_THINGS        =1,
    QUOTA_PROPS         =2,
    QUOTA_ARRAY_SIZE    =3,
    QUOTA_RAW_SIZE      =4,
};

struct ti_quota_s
{
    size_t max_things;
    size_t max_props;           /* max props per thing */
    size_t max_array_size;
    size_t max_raw_size;
};

ti_quota_t * ti_quota_create(void);
inline static void ti_quota_destroy(ti_quota_t * quota);
int ti_qouta_tp_from_strn(const char * str, size_t n, ex_t * e);

inline static void ti_quota_destroy(ti_quota_t * quota)
{
    free(quota);
}



#endif  /* TI_QUOTA_H_ */
