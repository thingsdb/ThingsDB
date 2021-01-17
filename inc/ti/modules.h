/*
 * ti/moduless.h
 */
#ifndef TI_MODULES_H_
#define TI_MODULES_H_


#include <ex.h>
#include <ti.h>
#include <ti/module.h>
#include <ti/raw.t.h>
#include <util/mpack.h>
#include <util/smap.h>


static inline ti_module_t * ti_modules_by_raw(ti_raw_t * raw)
{
    return smap_getn(ti.modules, (const char *) raw->data, raw->n);
}

void ti_modules_load(void);
void ti_modules_stop_and_destroy(void);

#endif /* TI_MODULES_H_ */
