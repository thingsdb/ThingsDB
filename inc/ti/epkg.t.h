/*
 * ti/epkg.t.h
 */
#ifndef TI_EPKG_T_H_
#define TI_EPKG_T_H_

typedef struct ti_epkg_s ti_epkg_t;

#include <inttypes.h>
#include <ti/pkg.t.h>

enum
{
    TI_EPKG_FLAG_ALLOW_GAP = 1<<0
};

struct ti_epkg_s
{
    uint32_t ref;
    uint32_t flags;     /* required for alignment with ti_rpkg_t */
    ti_pkg_t * pkg;     /* must align with ti_rpkg_t             */
    uint64_t event_id;
};

#endif  /* TI_EPKG_T_H_ */
