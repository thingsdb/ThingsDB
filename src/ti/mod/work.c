/*
 * ti/mod/work.c
 */
#include <ti/mod/work.h>

void ti_mod_work_destroy(ti_mod_work_t * data)
{
    ti_module_drop(data->module);
    free(data->buf.data);
    free(data);
}
