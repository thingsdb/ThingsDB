/*
 * ti/rpkg.h
 */
#ifndef TI_RPKG_H_
#define TI_RPKG_H_

#include <inttypes.h>
#include <ti/pkg.t.h>
#include <ti/rpkg.t.h>

ti_rpkg_t * ti_rpkg_create(ti_pkg_t * pkg);
void ti_rpkg_drop(ti_rpkg_t * rpkg);

#endif  /* TI_RPKG_H_ */
