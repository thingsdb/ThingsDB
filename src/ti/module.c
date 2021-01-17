/*
 * ti/module.c
 */
#include <ex.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/future.h>
#include <ti/module.h>
#include <ti/names.h>
#include <ti/pkg.h>
#include <ti/proc.h>
#include <ti/proto.t.h>
#include <ti/query.h>
#include <ti/val.inline.h>

#define MODULE__TOO_MANY_RESTARTS 15


static void module__write_req_cb(uv_write_t * req, int status)
{
    if (status)
    {
        ex_t e; /* TODO : new error? */
        ti_future_t * future = req->data;

        /* remove the future from the module */
        (void) omap_rm(future->module->futures, future->pid);

        ex_set(&e, EX_OPERATION_ERROR, uv_strerror(status));
        ti_query_on_future_result(future, &e);
    }

    free(req);
}

static int module__write_req(ti_future_t * future)
{
    int uv_err = 0;
    ti_module_t * module = future->module;
    ti_proc_t * proc = &module->proc;
    uv_buf_t wrbuf;
    ti_future_t * prev;
    uv_write_t * req;

    req = malloc(sizeof(uv_write_t));
    if (!req)
        return UV_EAI_MEMORY;

    req->data = future;
    future->pkg->id = future->pid = module->next_pid;

    prev = omap_set(module->futures, future->pid, future);
    if (!prev)
    {
        free(req);
        return UV_EAI_MEMORY;
    }

    if (prev != future)
        ti_future_cancel(prev);

    wrbuf = uv_buf_init(
            (char *) future->pkg,
            sizeof(ti_pkg_t) + future->pkg->n);

    uv_err = uv_write(
            req,
            (uv_stream_t *) &proc->child_stdin,
            &wrbuf,
            1,
            &module__write_req_cb);

    if (uv_err)
    {
        (void) omap_rm(module->futures, module->next_pid);
        free(req);
        return uv_err;
    }

    ++module->next_pid;
    return 0;
}

static void module__write_conf_cb(uv_write_t * req, int status)
{
    if (status)
    {
        log_error(uv_strerror(status));
        ((ti_module_t *) req->data)->status = status;
    }

    free(req);
}

static int module__write_conf(ti_module_t * module)
{
    int uv_err;
    ti_proc_t * proc = &module->proc;
    uv_buf_t wrbuf;
    uv_write_t * req;

    req = malloc(sizeof(uv_write_t));
    if (!req)
        return UV_EAI_MEMORY;

    req->data = module;

    wrbuf = uv_buf_init(
            (char *) module->conf_pkg,
            sizeof(ti_pkg_t) + module->conf_pkg->n);
    uv_err = uv_write(
            req,
            (uv_stream_t *) &proc->child_stdin,
            &wrbuf,
            1,
            &module__write_conf_cb);

    if (uv_err)
    {
        free(req);
        return uv_err;
    }

    return 0;
}

static void module__cb(ti_future_t * future)
{
    int uv_err;
    ti_thing_t * thing = VEC_get(future->args, 0);
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    size_t alloc_sz = 1024;

    assert (ti_val_is_thing((ti_val_t *) thing));

    if (mp_sbuffer_alloc_init(&buffer, alloc_sz, sizeof(ti_pkg_t)))
        goto mem_error0;

    if (ti_thing_to_pk(thing, &pk, future->deep))
        goto mem_error1;

    future->pkg = (ti_pkg_t *) buffer.data;
    pkg_init(future->pkg, 0, TI_PROTO_MODULE_REQ, buffer.size);

    uv_err = module__write_req(future);
    if (uv_err)
    {
        ex_t e;  /* TODO: introduce new error ? */
        ex_set(&e, EX_OPERATION_ERROR, uv_strerror(uv_err));
        ti_query_on_future_result(future, &e);
    }
    return;

mem_error1:
    msgpack_sbuffer_destroy(&buffer);
mem_error0:
    {
        ex_t e;
        ex_set_internal(&e);
        ti_query_on_future_result(future, &e);
    }
}

ti_pkg_t * ti_module_conf_pkg(ti_val_t * val)
{
    ti_pkg_t * pkg;
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    size_t alloc_sz = 1024;

    if (mp_sbuffer_alloc_init(&buffer, alloc_sz, sizeof(ti_pkg_t)))
        return NULL;

    if (ti_val_to_pk(val, &pk, 1))
    {
        msgpack_sbuffer_destroy(&buffer);
        return NULL;
    }

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_MODULE_CONF, buffer.size);

    return pkg;
}

ti_module_t * ti_module_create(
        const char * name,
        size_t name_n,
        const char * binary,
        size_t binary_n,
        uint64_t created_at,
        ti_pkg_t * conf_pkg,    /* may be NULL */
        uint64_t * scope_id     /* may be NULL */)
{
    ti_module_t * module = malloc(sizeof(ti_module_t));
    if (!module)
        return NULL;

    module->status = TI_MODULE_STAT_NOT_LOADED;
    module->restarts = 0;
    module->next_pid = 0;
    module->cb = (ti_module_cb) &module__cb;
    module->name = ti_names_get(name, name_n);
    module->binary = ti_names_get(binary, binary_n);
    module->conf_pkg = conf_pkg;
    module->started_at = 0;
    module->created_at = created_at;
    module->scope_id = scope_id;
    module->futures = omap_create();

    if (!module->name || !module->binary || !module->futures ||
        smap_add(ti.modules, module->name->str, module))
    {
        ti_module_destroy(module);
        return NULL;
    }

    ti_proc_init(&module->proc, module);

    return module;
}

int ti_module_load(ti_module_t * module)
{
    int rc = ti_proc_load(&module->proc);

    module->status = rc
            ? rc
            : module->conf_pkg
            ? module__write_conf(module)
            : TI_MODULE_STAT_RUNNING;

    return module->status;
}

void ti_module_destroy(ti_module_t * module)
{
    if (!module)
        return;
    ti_val_drop((ti_val_t *) module->name);
    ti_val_drop((ti_val_t *) module->binary);
    omap_destroy(module->futures, (omap_destroy_cb) ti_future_cancel);
    free(module->conf_pkg);
    free(module);
}

void ti_module_on_exit(ti_module_t * module)
{
    omap_clear(module->futures, (omap_destroy_cb) ti_future_cancel);

    if (module->status == TI_MODULE_STAT_RUNNING &&
        (~ti.flags & TI_FLAG_SIGNAL))
    {
        if (++module->restarts > MODULE__TOO_MANY_RESTARTS)
        {
            log_error(
                    "module `%s` has been restarted too many (>%d) times",
                    module->binary->str,
                    MODULE__TOO_MANY_RESTARTS);
            module->status = TI_MODULE_STAT_TOO_MANY_RESTARTS;
        }
        else if (ti_module_load(module))
        {
            log_error(
                    "failed to start module `%s` (%s)",
                    module->binary->str,
                    ti_module_status_str(module));
        }
    }
}

void ti_module_stop(ti_module_t * module)
{
    int rc, pid = module->proc.process.pid;
    if (!pid)
        return;
    rc = uv_kill(pid, SIGTERM);
    if (rc)
        log_error("BLA");
    /* TODO: stop... */
}

static void module__on_res(
        ti_module_t * module,
        ti_future_t * future,
        ti_pkg_t * pkg)
{
    mp_unp_init(&up, pkg->data, pkg->n);

}

static void module__on_err(
        ti_module_t * module,
        ti_future_t * future,
        ti_pkg_t * pkg)
{
    ex_t e = {0};
    mp_unp_t up;
    mp_obj_t mp_obj, mp_err_code, mp_err_msg;
    int64_t err_code;

    mp_unp_init(&up, pkg->data, pkg->n);

    if (mp_next(&up, &mp_obj) != MP_ARR || mp_obj.via.sz != 2 ||
        mp_next(&up, &mp_err_code) != MP_I64 ||
        mp_next(&up, &mp_err_msg) != MP_STR)
    {
        ex_set(&e, EX_BAD_DATA,
                "invalid error data from module `%s`",
                module->binary->str);
        goto done;
    }

    if (mp_err_code.via.i64 < EX_MIN_ERR ||
        mp_err_code.via.i64 > EX_MAX_BUILD_IN_ERR)
    {
        ex_set(e, EX_BAD_DATA,
            "invalid error code (%"PRId64") received from module `%s`",
            mp_err_code.via.i64,
            module->binary->str);
        goto done;
    }

    if (ti_verror_check_msg(mp_err_msg.via.str.data, mp_err_msg.via.str.n, &e))
        goto done;

    ex_setn(e, mp_err_code.via.i64, mp_err_msg.via.str, mp_err_msg.via.str.n);

done:
    ti_query_on_future_result(future, &e);
}

void ti_module_on_pkg(ti_module_t * module, ti_pkg_t * pkg)
{
    ti_future_t * future = omap_rm(module->futures, pkg->id);
    if (!future)
    {
        log_error(
                "got a response for future id %u but a future with this id "
                "does not exist; maybe the future has been cancelled?",
                pkg->id);
        return;
    }

    switch(pkg->tp)
    {
    case TI_PROTO_MODULE_RES:
        module__on_res(module, future, pkg);
        return;
    case TI_PROTO_MODULE_ERR:
        module__on_err(module, future, pkg);
        return;
    default:
    {
        ex_t e;
        ex_set(e, EX_BAD_DATA,
                "unexpected package type `%s` (%u) from module `%s`",
                ti_proto_str(pkg->tp), pkg->tp, module->binary->str);
        ti_query_on_future_result(future, &e);
    }
    }
}

const char * ti_module_status_str(ti_module_t * module)
{
    switch (module->status)
    {
    case TI_MODULE_STAT_RUNNING:
        return "running";
    case TI_MODULE_STAT_NOT_LOADED:
        return "module not loaded";
    case TI_MODULE_STAT_STOPPING:
        return "stopping";
    case TI_MODULE_STAT_TOO_MANY_RESTARTS:
        return "too many restarts detected; most likely the module is broken";
    }
    return uv_strerror(module->status);
}

