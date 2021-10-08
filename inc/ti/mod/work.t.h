/*
 * ti/mod/work.t.h
 */
#ifndef TI_MOD_WORK_T_H_
#define TI_MOD_WORK_T_H_

typedef struct ti_mod_work_s ti_mod_work_t;

#include <ti/mod/manifest.t.h>
#include <ti/module.t.h>
#include <util/buf.h>
#include <uv.h>


struct ti_mod_work_s
{
    ti_module_t * module;
    buf_t buf;
    ti_mod_manifest_t manifest;
    void (*download_cb) (uv_work_t *);
    _Bool rtxt;                             /* Python, requirements.txt */
};

#endif  /* TI_MOD_WORK_T_H_ */
