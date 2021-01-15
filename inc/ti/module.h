/*
 * ti/module.h
 */
#ifndef TI_MODULE_H_
#define TI_MODULE_H_

#include <ti/module.t.h>

ti_module_t * ti_module_create_strn(
        const char * name,
        size_t name_n,
        const char * binary,
        size_t binary_n,
        const char * conf,
        size_t conf_n,
        uint64_t created_at,
        ti_scope_t * scope /* may be NULL */);

void ti_module_destroy(ti_module_t * module);
void ti_module_on_exit(uv_process_t * process);
void ti_module_stop(ti_module_t * module);
const char * ti_module_status_str(ti_module_t * module);

#endif /* TI_MODULE_H_ */
