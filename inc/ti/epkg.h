/*
 * ti/epkg.h
 */
#ifndef TI_EPKG_H_
#define TI_EPKG_H_

typedef struct ti_epkg_s ti_epkg_t;

#include <inttypes.h>
#include <ti/pkg.h>
#include <ti/rpkg.h>

ti_epkg_t * ti_epkg_create(ti_pkg_t * pkg, uint64_t event_id);
ti_epkg_t * ti_epkg_initial(void);
ti_epkg_t * ti_epkg_from_pkg(ti_pkg_t * pkg);
static inline void ti_epkg_drop(ti_epkg_t * epkg);

struct ti_epkg_s
{
    uint32_t ref;
    uint32_t pad0;      /* required for alignment with ti_rpkg_t */
    ti_pkg_t * pkg;     /* must align with ti_rpkg_t             */
    uint64_t event_id;
};

static inline void ti_epkg_drop(ti_epkg_t * epkg)
{
    ti_rpkg_drop((ti_rpkg_t *) epkg);
}

#endif  /* TI_EPKG_H_ */
