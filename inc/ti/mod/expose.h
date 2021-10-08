/*
 * ti/mod/expose.h
 */
#ifndef TI_MOD_EXPOSE_H_
#define TI_MOD_EXPOSE_H_

#include <cleri/cleri.h>
#include <ex.h>
#include <ti/mod/expose.t.h>
#include <ti/module.t.h>
#include <ti/query.t.h>
#include <ti/vp.t.h>
#include <util/smap.h>

ti_mod_expose_t * ti_mod_expose_create(void);
void ti_mod_expose_destroy(ti_mod_expose_t * expose);
int ti_mod_expose_call(
        ti_mod_expose_t * expose,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e);
int ti_mod_expose_info_to_vp(
        const char * key,
        size_t n,
        ti_mod_expose_t * expose,
        ti_vp_t * vp);

static inline ti_mod_expose_t * ti_mod_expose_by_strn(
        ti_module_t * module,
        const char * s,
        size_t n)
{
    return module->manifest.exposes
            ? smap_getn(module->manifest.exposes, s, n)
            : NULL;
}

#endif  /* TI_MOD_EXPOSE_H_ */
