/*
 * ti/modu/work.t.h
 */
#ifndef TI_MODU_WORK_T_H_
#define TI_MODU_WORK_T_H_

typedef struct ti_modu_work_s ti_modu_work_t;

#include <ti/modu/manifest.t.h>
#include <ti/module.t.h>
#include <util/buf.h>
#include <uv.h>


struct ti_modu_work_s
{
    ti_module_t * module;
    buf_t buf;
    ti_modu_manifest_t manifest;
    void (*download_cb) (uv_work_t *);
    _Bool rtxt;                             /* Python, requirements.txt */
};

#endif  /* TI_MODU_WORK_T_H_ */
