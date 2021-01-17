/*
 * ti/module.h
 */
#ifndef TI_MODULE_H_
#define TI_MODULE_H_

#include <ti/pkg.t.h>
#include <ti/val.t.h>
#include <ti/scope.t.h>
#include <ti/module.t.h>

ti_module_t * ti_module_create(
        const char * name,
        size_t name_n,
        const char * binary,
        size_t binary_n,
        uint64_t created_at,
        ti_pkg_t * conf_pkg,    /* may be NULL */
        uint64_t * scope_id     /* may be NULL */);

void ti_module_destroy(ti_module_t * module);
void ti_module_on_exit(ti_module_t * module);
void ti_module_stop(ti_module_t * module);
void ti_module_load(ti_module_t * module);
const char * ti_module_status_str(ti_module_t * module);
ti_pkg_t * ti_module_conf_pkg(ti_val_t * val);
void ti_module_on_pkg(ti_module_t * module, ti_pkg_t * pkg);

#endif /* TI_MODULE_H_ */
