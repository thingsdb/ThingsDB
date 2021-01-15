/*
 * ti/module.c
 */
#include <stdlib.h>
#include <ti/module.h>

ti_module_t * ti_module_create_strn(
        const char * name,
        size_t name_n,
        const char * binary,
        size_t binary_n,
        const char * conf,
        size_t conf_n,
        uint64_t created_at,
        ti_scope_t * scope /* may be NULL */)
{
    ti_module_t * module = malloc(sizeof(ti_module_t));
    if (!module)
        return NULL;

    module->status = TI_MODULE_STAT_NOT_LOADED;
    module->restarts = 0;
    module->next_pid = 0;
    module->cb = ti_module_cb;
    module->name = ti_str_create(name, name_n);
    module->binary = ti_str_create(binary, binary_n);
    module->conf = conf_n ? ti_raw_create(conf, conf_n) : NULL;
    module->started_at = 0;
    module->created_at = created_at;
    module->scope = scope;
    module->futures = omap_create();

    if ((conf_n && !module->conf) ||
        !module->name || !module->binary || !module->futures)
    {
        ti_module_destroy(module);
        return NULL;
    }
    return module;
}

int ti_module_load(ti_module_t * module)
{

}

void ti_module_destroy(ti_module_t * module)
{
    if (!module)
        return;
    assert (module->status != TI_MODULE_STAT_RUNNING);

    ti_val_drop((ti_val_t *) module->name);
    ti_val_drop((ti_val_t *) module->binary);
    ti_val_drop((ti_val_t *) module->conf);
    omap_destroy(module->futures, (omap_destroy_cb) ti_future_cancel);
    free(module);
}

void ti_module_stop(ti_module_t * module)
{

}

void ti_module_cb(ti_future_t * future)
{
    int uv_err = 0;
    ti_thing_t * thing = VEC_get(future->args, 0);
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    size_t alloc_sz = 1024;
    ti_pkg_t * pkg = NULL;
    uv_write_t * req;
    uv_buf_t wrbuf;
    ti_module_t * module = future->module;
    ti_proc_t * proc = &module->proc;
    ti_future_t * prev;

    assert (ti_val_is_thing((ti_val_t *) thing));

    if (mp_sbuffer_alloc_init(&buffer, alloc_sz, sizeof(ti_pkg_t)))
        goto mem_error0;

    if (ti_thing_to_pk(thing, &pk, future->deep))
        goto mem_error1;

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_MODULE_REQ, buffer.size);

    req = malloc(sizeof(uv_write_t));
    if (!req)
        goto mem_error1;

    req->data = future;
    future->pid = module->next_pid;

    prev = omap_set(module->futures, future->pid, future);
    if (!prev)
        goto mem_error2;

    if (prev != future)
    {
        ex_t e;  /* TODO: introduce new error ? */
        ex_set(e, EX_OPERATION_ERROR, "future cancelled before completion");
        ti_query_on_future_result(prev, &e);
    }

    wrbuf = uv_buf_init((char *) pkg, sizeof(ti_pkg_t) + pkg->n);

    uv_err = ti_proc_write_request(proc, req, &wrbuf);

    if (uv_err)
    {
        ex_t e;  /* TODO: introduce new error ? */
        ex_set(e, EX_OPERATION_ERROR, uv_strerror(uv_err));
        ti_query_on_future_result(future, &e);
        goto uv_error;
    }

    ++module->next_pid;
    return;  /* success */

uv_error:
    omap_rm(module->futures, module->next_pid);
mem_error2:
    free(req);
mem_error1:
    msgpack_sbuffer_destroy(buffer);
mem_error0:
    if (uv_err == 0)
    {
        ex_t e;
        ex_set_internal(&e);
        ti_query_on_future_result(future, &e);
    }
}

