/*
 * ti/cpkg.h
 */
#ifndef TI_CPKG_H_
#define TI_CPKG_H_

#include <inttypes.h>
#include <ti/cpkg.t.h>
#include <ti/pkg.t.h>
#include <ti/rpkg.t.h>

ti_cpkg_t * ti_cpkg_create(ti_pkg_t * pkg, uint64_t change_id);
ti_cpkg_t * ti_cpkg_initial(void);
ti_cpkg_t * ti_cpkg_from_pkg(ti_pkg_t * pkg);

#endif  /* TI_CPKG_H_ */
