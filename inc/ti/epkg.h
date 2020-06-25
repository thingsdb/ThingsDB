/*
 * ti/epkg.h
 */
#ifndef TI_EPKG_H_
#define TI_EPKG_H_

#include <inttypes.h>
#include <ti/epkg.t.h>
#include <ti/pkg.t.h>
#include <ti/rpkg.t.h>

ti_epkg_t * ti_epkg_create(ti_pkg_t * pkg, uint64_t event_id);
ti_epkg_t * ti_epkg_initial(void);
ti_epkg_t * ti_epkg_from_pkg(ti_pkg_t * pkg);

#endif  /* TI_EPKG_H_ */
