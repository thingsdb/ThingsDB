/*
 * ti/epkg.inline.h
 */
#ifndef TI_EPKG_INLINE_H_
#define TI_EPKG_INLINE_H_

#include <ti/rpkg.h>
#include <ti/epkg.t.h>

static inline void ti_epkg_drop(ti_epkg_t * epkg)
{
    ti_rpkg_drop((ti_rpkg_t *) epkg);
}

#endif  /* TI_EPKG_INLINE_H_ */
