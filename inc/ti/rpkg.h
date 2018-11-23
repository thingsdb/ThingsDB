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
    ti_pkg_t * pkg;
};

#endif  /* TI_RPKG_H_ */
