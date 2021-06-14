/*
 * ti/modules.c
 */
#include <ti/modules.h>
#include <ti/varr.h>
#include <ti/names.h>
#include <ti/val.inline.h>

/*
 * This function tries to create the module path if it does not exist.
 * It's not at all critical when this function fails to create the path, the
 * as a result the module path will simply not exist.
 */
void ti_modules_init(void)
{
    char * path, * modules_path = ti.cfg->modules_path;

    if (!fx_is_dir(modules_path) && mkdir(modules_path, FX_DEFAULT_DIR_ACCESS))
    {
        log_errno_file("cannot create directory", errno, modules_path);
        goto do_python;
    }

    path = realpath(ti.cfg->modules_path, NULL);
    if (!path)
    {
        log_warning("cannot find storage path: `%s`", path);
        goto do_python;
    }

    free(ti.cfg->modules_path);
    ti.cfg->modules_path = path;

do_python:
    /* Just log information about the python interpreter */
    if (fx_is_executable(ti.cfg->python_interpreter))
        return;  /* done */

    path = fx_get_executable_in_path(ti.cfg->python_interpreter);
    if (!path)
    {
        log_warning(
                "cannot find Python interpreter: `%s`",
                ti.cfg->python_interpreter);
        return;
    }

    free(ti.cfg->python_interpreter);
    ti.cfg->python_interpreter = path;
}

static int modules__load_cb(ti_module_t * module, void * UNUSED(arg))
{
    if (module->status == TI_MODULE_STAT_NOT_LOADED)
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

int ti_modules_rename(ti_module_t * module, const char * s, size_t n)
{
    ti_name_t * tmp = ti_names_get(s, n);

    if (!tmp || smap_add(ti.modules, tmp->str, module))
    {
        ti_val_drop((ti_val_t *) tmp);
        return -1;
    }
    (void) smap_pop(ti.modules, module->name->str);

    ti_val_unsafe_drop((ti_val_t *) module->name);
    module->name = tmp;
    return 0;
}

static int modues__info_cb(ti_module_t * module, ti_varr_t * varr)
{
    ti_val_t * mpinfo = ti_module_as_mpval(module, varr->flags);
    if (!mpinfo)
        return -1;
    VEC_push(varr->vec, mpinfo);
    return 0;
}

ti_varr_t * ti_modules_info(_Bool with_conf)
{
    uint8_t flags;
    ti_varr_t * varr = ti_varr_create(ti.modules->n);
    if (!varr)
        return NULL;

    flags = varr->flags;
    varr->flags = with_conf;

    if (smap_values(ti.modules, (smap_val_cb) modues__info_cb, varr))
    {
        ti_val_unsafe_drop((ti_val_t *) varr);
        return NULL;
    }

    varr->flags = flags;
    return varr;
}

static int modules__cfuture_cb(ti_module_t * module, void * UNUSED(arg))
{
    ti_module_cancel_futures(module);
    return 0;
}

void ti_modules_cancel_futures(void)
{
    (void) smap_values(ti.modules, (smap_val_cb) modules__cfuture_cb, NULL);
}
