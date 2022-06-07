/*
 * ti/cpkg.t.h
 */
#ifndef TI_CPKG_T_H_
#define TI_CPKG_T_H_

typedef struct ti_cpkg_s ti_cpkg_t;

#include <inttypes.h>
#include <ti/pkg.t.h>

enum
{
    TI_CPKG_FLAG_ALLOW_GAP = 1<<0
};

struct ti_cpkg_s
{
    uint32_t ref;
    uint32_t flags;     /* required for alignment with ti_rpkg_t */
    ti_pkg_t * pkg;     /* must align with ti_rpkg_t             */
    uint64_t change_id;
};

#endif  /* TI_CPKG_T_H_ */
