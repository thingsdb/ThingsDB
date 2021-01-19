/*
 * ti/moduless.h
 */
#ifndef TI_MODULES_H_
#define TI_MODULES_H_


#include <ex.h>
#include <ti.h>
#include <ti/module.h>
#include <ti/raw.t.h>
#include <ti/varr.t.h>
#include <util/mpack.h>
#include <util/smap.h>

ti_varr_t * ti_modules_info(_Bool with_conf);
void ti_modules_cancel_futures(void);

static inline ti_module_t * ti_modules_by_raw(ti_raw_t * raw)
{
    return smap_getn(ti.modules, (const char *) raw->data, raw->n);
}

static inline ti_module_t * ti_modules_by_strn(const char * s, size_t n)
{
    return smap_getn(ti.modules, s, n);
}

void ti_modules_load(void);
void ti_modules_stop_and_destroy(void);

#endif /* TI_MODULES_H_ */
