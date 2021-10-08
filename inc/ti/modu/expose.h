/*
 * ti/modu/expose.h
 */
#ifndef TI_MODU_EXPOSE_H_
#define TI_MODU_EXPOSE_H_

#include <cleri/cleri.h>
#include <ex.h>
#include <ti/modu/expose.t.h>
#include <ti/module.t.h>
#include <ti/query.t.h>
#include <ti/vp.t.h>
#include <util/smap.h>

ti_modu_expose_t * ti_modu_expose_create(void);
void ti_modu_expose_destroy(ti_modu_expose_t * expose);
int ti_modu_expose_call(
        ti_modu_expose_t * expose,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e);
int ti_modu_expose_info_to_vp(
        const char * key,
        size_t n,
        ti_modu_expose_t * expose,
        ti_vp_t * vp);

static inline ti_modu_expose_t * ti_modu_expose_by_strn(
        ti_module_t * module,
        const char * s,
        size_t n)
{
    return module->manifest.exposes
            ? smap_getn(module->manifest.exposes, s, n)
            : NULL;
}

#endif  /* TI_MODU_EXPOSE_H_ */
