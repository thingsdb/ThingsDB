/*
 * ti/rpkg.h
 */
#ifndef TI_RPKG_H_
#define TI_RPKG_H_

typedef struct ti_rpkg_s ti_rpkg_t;

#include <inttypes.h>
#include <ti/pkg.h>

ti_rpkg_t * ti_rpkg_create(ti_pkg_t * pkg);
void ti_rpkg_drop(ti_rpkg_t * rpkg);

struct ti_rpkg_s
{
    uint32_t ref;
    uint32_t pad0;      /* required for alignment with ti_epkg_t */
    ti_pkg_t * pkg;     /* must align with ti_epkg_t             */
};

#endif  /* TI_RPKG_H_ */
