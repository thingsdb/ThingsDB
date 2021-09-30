/*
 * ti/mod/work.t.h
 */
#ifndef TI_MOD_WORK_T_H_
#define TI_MOD_WORK_T_H_

typedef struct ti_mod_work_s ti_mod_work_t;

#include <ti/module.t.h>
#include <ti/mod/manifest.t.h>

struct ti_mod_work_s
{
    ti_module_t * module;
    ti_mod_manifest_t manifest;
};


#endif  /* TI_MOD_WORK_T_H_ */
