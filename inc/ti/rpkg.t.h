/*
 * ti/rpkg.t.h
 */
#ifndef TI_RPKG_T_H_
#define TI_RPKG_T_H_

typedef struct ti_rpkg_s ti_rpkg_t;

#include <inttypes.h>
#include <ti/pkg.t.h>

struct ti_rpkg_s
{
    uint32_t ref;
    long:32;            /* required for alignment with ti_cpkg_t */
    ti_pkg_t * pkg;     /* must align with ti_cpkg_t             */
};

#endif  /* TI_RPKG_T_H_ */
