/*
 * quota.h
 */
#ifndef TI_QUOTA_H_
#define TI_QUOTA_H_

#include <stddef.h>
#include <stdlib.h>
#include <ti/ex.h>
#include <qpack.h>

typedef struct ti_quota_s ti_quota_t;

#define TI_QUOTA_NOT_SET SIZE_MAX

typedef enum
{
    QUOTA_THINGS        =1,
    QUOTA_PROPS         =2,
    QUOTA_ARRAY_SIZE    =3,
    QUOTA_RAW_SIZE      =4,
} ti_quota_enum_t;

struct ti_quota_s
{
    size_t max_things;
    size_t max_props;           /* max props per thing */
    size_t max_array_size;
    size_t max_raw_size;
};

ti_quota_t * ti_quota_create(void);
ti_quota_enum_t ti_qouta_tp_from_strn(const char * str, size_t n, ex_t * e);
int ti_quota_val_to_packer(qp_packer_t * packer, size_t quota);
inline static void ti_quota_destroy(ti_quota_t * quota);

inline static void ti_quota_destroy(ti_quota_t * quota)
{
    free(quota);
}

#endif  /* TI_QUOTA_H_ */
