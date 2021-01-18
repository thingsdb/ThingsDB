/*
 * ti/modules.c
 */
#include <ti/modules.h>

static int modules__load_cb(ti_module_t * module, void * UNUSED(arg))
{
    ti_module_load(module);
    return 0;
}

void ti_modules_load(void)
{
    (void) smap_values(ti.modules, (smap_val_cb) modules__load_cb, NULL);
}

void ti_modules_stop_and_destroy(void)
{
    smap_destroy(ti.modules, (smap_destroy_cb) ti_module_stop_and_destroy);
    ti.modules = NULL;
}
