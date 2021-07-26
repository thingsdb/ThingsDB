/*
 * ti/cpkg.inline.h
 */
#ifndef TI_EPKG_INLINE_H_
#define TI_EPKG_INLINE_H_

#include <ti/rpkg.h>
#include <ti/cpkg.t.h>

static inline void ti_cpkg_drop(ti_cpkg_t * cpkg)
{
    ti_rpkg_drop((ti_rpkg_t *) cpkg);
}

#endif  /* TI_EPKG_INLINE_H_ */
