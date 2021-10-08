/*
 * ti/modu/work.c
 */
#include <ti/modu/manifest.h>
#include <ti/modu/work.h>
#include <ti/module.h>

void ti_modu_work_destroy(ti_modu_work_t * data)
{
    ti_modu_manifest_clear(&data->manifest);
    ti_module_drop(data->module);
    free(data->buf.data);
    free(data);
}
