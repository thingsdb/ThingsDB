/*
 * ti/modules.h
 */
#ifndef TI_MODULE_H_
#define TI_MODULE_H_


#include <ex.h>
#include <ti/module.h>
#include <ti/raw.t.h>
#include <util/mpack.h>
#include <util/smap.h>


static inline ti_module_t * ti_modules_by_raw(ti_raw_t * raw)
{
    return smap_getn(ti.modules, (const char *) raw->data, raw->n);
}


#endif /* TI_MODULE_H_ */
