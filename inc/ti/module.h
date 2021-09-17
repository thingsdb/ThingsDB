/*
 * ti/module.h
 */
#ifndef TI_MODULE_H_
#define TI_MODULE_H_

#include <cleri/cleri.h>
#include <ex.h>
#include <ti/module.t.h>
#include <ti/pkg.t.h>
#include <ti/query.t.h>
#include <ti/scope.t.h>
#include <ti/thing.t.h>
#include <ti/val.t.h>
#include <util/fx.h>
#include <util/mpack.h>

ti_module_t * ti_module_create(
        const char * name,
        size_t name_n,
        const char * file,
        size_t file_n,
        uint64_t created_at,
        ti_pkg_t * conf_pkg,    /* may be NULL */
        uint64_t * scope_id     /* may be NULL */);
int ti_module_validate_file(const char * file, size_t file_n, ex_t * e);
void ti_module_destroy(ti_module_t * module);
void ti_module_on_exit(ti_module_t * module);
int ti_module_stop(ti_module_t * module);
void ti_module_stop_and_destroy(ti_module_t * module);
void ti_module_del(ti_module_t * module);
void ti_module_cancel_futures(ti_module_t * module);
void ti_module_load(ti_module_t * module);
void ti_module_restart(ti_module_t * module);
void ti_module_update_conf(ti_module_t * module);
const char * ti_module_status_str(ti_module_t * module);
ti_pkg_t * ti_module_conf_pkg(ti_val_t * val, ti_query_t * query);
void ti_module_on_pkg(ti_module_t * module, ti_pkg_t * pkg);
int ti_module_info_to_pk(
        ti_module_t * module,
        msgpack_packer * pk,
        int options);
ti_val_t * ti_module_as_mpval(ti_module_t * module, int flags);
int ti_module_write(ti_module_t * module, const void * data, size_t n);
int ti_module_read_args(
        ti_thing_t * thing,
        _Bool * load,
        uint8_t * deep,
        ex_t * e);
int ti_module_call(
        ti_module_t * module,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e);

static inline _Bool ti_module_is_py(ti_module_t * module)
{
    return module->flags & TI_MODULE_FLAG_IS_PY_MODULE;
}

static inline const char * ti_module_py_fn(ti_module_t * module)
{
    return module->args[1];
}

static inline void ti_module_drop(ti_module_t * module)
{
    if (!--module->ref)
        ti_module_destroy(module);
}

#endif /* TI_MODULE_H_ */
