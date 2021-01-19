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
#include <ti/proto.h>
#include <ti/proto.t.h>
#include <ti/query.h>
#include <ti/scope.h>
#include <ti/val.inline.h>
#include <ti/verror.h>
#include <util/fx.h>

#define MODULE__TOO_MANY_RESTARTS 15


static void module__write_req_cb(uv_write_t * req, int status)
{
    if (status)
    {
        ex_t e;
        ti_future_t * future = req->data;

        /* remove the future from the module */
        (void) omap_rm(future->module->futures, future->pid);

        ex_set(&e, EX_OPERATION, uv_strerror(status));
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

    if (future->module->status)
    {
        ex_t e;
        ex_set(&e, EX_OPERATION, ti_module_status_str(future->module));
        ti_query_on_future_result(future, &e);
        return;
    }

    if (mp_sbuffer_alloc_init(&buffer, alloc_sz, sizeof(ti_pkg_t)))
        goto mem_error0;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (ti_thing_to_pk(thing, &pk, future->deep))
        goto mem_error1;

    future->pkg = (ti_pkg_t *) buffer.data;
    pkg_init(future->pkg, 0, TI_PROTO_MODULE_REQ, buffer.size);

    uv_err = module__write_req(future);
    if (uv_err)
    {
        ex_t e;
        ex_set(&e, EX_OPERATION, uv_strerror(uv_err));
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
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

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
        const char * file,
        size_t file_n,
        uint64_t created_at,
        ti_pkg_t * conf_pkg,    /* may be NULL */
        uint64_t * scope_id     /* may be NULL */)
{
    ti_module_t * module = malloc(sizeof(ti_module_t));
    if (!module)
        return NULL;

    module->status = TI_MODULE_STAT_NOT_LOADED;
    module->flags = 0;
    module->restarts = 0;
    module->next_pid = 0;
    module->cb = (ti_module_cb) &module__cb;
    module->name = ti_names_get(name, name_n);
    module->file = fx_path_join_strn(
            ti.cfg->modules_path,
            strlen(ti.cfg->modules_path),
            file,
            file_n);
    module->args = malloc(sizeof(char*) * 2);
    module->conf_pkg = conf_pkg;
    module->started_at = 0;
    module->created_at = created_at;
    module->scope_id = scope_id;
    module->futures = omap_create();

    if (!module->name || !module->file || !module->futures || !module->args ||
        smap_add(ti.modules, module->name->str, module))
    {
        ti_module_destroy(module);
        return NULL;
    }

    module->fn = module->file + (strlen(module->file) - file_n);
    module->args[0] = module->file;
    module->args[0] = NULL;

    ti_proc_init(&module->proc, module);

    return module;
}

void ti_module_load(ti_module_t * module)
{
    int rc;

    if (module->flags & TI_MODULE_FLAG_IN_USE)
    {
        log_debug(
                "module `%s` already loaded (PID %d)",
                module->name->str, module->proc.process.pid);
        return;
    }

    rc = ti_proc_load(&module->proc);

    module->status = rc
            ? rc
            : module->conf_pkg
            ? module__write_conf(module)
            : TI_MODULE_STAT_RUNNING;

    if (module->status == TI_MODULE_STAT_RUNNING)
        log_info(
                "loaded module `%s` (%s)",
                module->name->str,
                module->file);
    else
        log_error(
                "failed to start module `%s` (%s): %s",
                module->name->str,
                module->file,
                ti_module_status_str(module));
}

void ti_module_destroy(ti_module_t * module)
{
    if (!module)
        return;
    omap_destroy(module->futures, (omap_destroy_cb) ti_future_cancel);
    ti_val_drop((ti_val_t *) module->name);
    free(module->file);
    free(module->args);
    free(module->conf_pkg);
    free(module->scope_id);
    free(module);
}

void ti_module_on_exit(ti_module_t * module)
{
    /* first cancel all open futures */
    ti_modules_cancel_futures(module);

    if (module->flags & TI_MODULE_FLAG_DESTROY)
    {
        ti_module_destroy(module);
        return;
    }

    module->flags &= ~TI_MODULE_FLAG_IN_USE;

    if (module->status == TI_MODULE_STAT_RUNNING)
    {
        if (++module->restarts > MODULE__TOO_MANY_RESTARTS)
        {
            log_error(
                    "module `%s` has been restarted too many (>%d) times",
                    module->name->str,
                    MODULE__TOO_MANY_RESTARTS);
            module->status = TI_MODULE_STAT_TOO_MANY_RESTARTS;
        }
        else
            ti_module_load(module);
        return;
    }

}

int ti_module_stop(ti_module_t * module)
{
    int rc, pid = module->proc.process.pid;
    if (!pid)
        return 0;
    rc = uv_kill(pid, SIGTERM);
    if (rc)
    {
        log_error(
                "failed to stop module `%s` (%s)",
                module->name->str,
                uv_strerror(rc));
        module->status = rc;
    }
    else
        module->status = TI_MODULE_STAT_STOPPING;
    return rc;
}

void ti_module_stop_and_destroy(ti_module_t * module)
{
    module->flags |= TI_MODULE_FLAG_DESTROY;
    if (module->flags & TI_MODULE_FLAG_IN_USE)
        (void) ti_module_stop(module);
    else
        ti_module_destroy(module);
}

void ti_module_del(ti_module_t * module)
{
    ti_module_stop_and_destroy(smap_pop(ti.modules, module->name->str));
}

void ti_module_cancel_futures(ti_module_t * module)
{
    omap_clear(module->futures, (omap_destroy_cb) ti_future_cancel);
}

static void module__on_res(ti_future_t * future, ti_pkg_t * pkg)
{
    ex_t e = {0};
    ti_val_t * val;
    mp_unp_t up;
    ti_vup_t vup = {
            .isclient = true,
            .collection = future->query->collection,
            .up = &up,
    };
    mp_unp_init(&up, pkg->data, pkg->n);
    val = ti_val_from_vup_e(&vup, &e);
    if (!val)
    {
        ti_query_on_future_result(future, &e);
        return;
    }

    ti_val_unsafe_drop(vec_set(future->args, val, 0));
    future->rval = (ti_val_t *) ti_varr_from_vec(future->args);
    if (!future->rval)
        ex_set_mem(&e);
    else
        future->args = NULL;

    ti_query_on_future_result(future, &e);
}

static void module__on_err(ti_future_t * future, ti_pkg_t * pkg)
{
    ex_t e = {0};
    mp_unp_t up;
    mp_obj_t mp_obj, mp_err_code, mp_err_msg;

    mp_unp_init(&up, pkg->data, pkg->n);

    if (mp_next(&up, &mp_obj) != MP_ARR || mp_obj.via.sz != 2 ||
        mp_next(&up, &mp_err_code) != MP_I64 ||
        mp_next(&up, &mp_err_msg) != MP_STR)
    {
        ex_set(&e, EX_BAD_DATA,
                "invalid error data from module `%s`",
                future->module->name->str);
        goto done;
    }

    if (mp_err_code.via.i64 < EX_MIN_ERR ||
        mp_err_code.via.i64 > EX_MAX_BUILD_IN_ERR)
    {
        ex_set(&e, EX_BAD_DATA,
            "invalid error code (%"PRId64") received from module `%s`",
            mp_err_code.via.i64,
            future->module->name->str);
        goto done;
    }

    if (ti_verror_check_msg(mp_err_msg.via.str.data, mp_err_msg.via.str.n, &e))
        goto done;

    ex_setn(&e,
            mp_err_code.via.i64,
            mp_err_msg.via.str.data,
            mp_err_msg.via.str.n);

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
        module__on_res(future, pkg);
        return;
    case TI_PROTO_MODULE_ERR:
        module__on_err(future, pkg);
        return;
    default:
    {
        ex_t e;
        ex_set(&e, EX_BAD_DATA,
                "unexpected package type `%s` (%u) from module `%s`",
                ti_proto_str(pkg->tp), pkg->tp, module->name->str);
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
        return "stopping module...";
    case TI_MODULE_STAT_TOO_MANY_RESTARTS:
        return "too many restarts detected; most likely the module is broken";
    }
    return uv_strerror(module->status);
}

int ti_module_info_to_pk(
        ti_module_t * module,
        msgpack_packer * pk,
        _Bool with_conf)
{
    return -(
        msgpack_pack_map(pk, 5 + !!with_conf) ||

        mp_pack_str(pk, "name") ||
        mp_pack_strn(pk, module->name->str, module->name->n) ||

        mp_pack_str(pk, "file") ||
        mp_pack_str(pk, module->file) ||

        mp_pack_str(pk, "created_at") ||
        msgpack_pack_uint64(pk, module->created_at) ||

        (with_conf && (
                mp_pack_str(pk, "conf") ||
                (module->conf_pkg
                        ? mp_pack_append(
                            pk,
                            module->conf_pkg + sizeof(ti_pkg_t),
                            module->conf_pkg->n)
                        : msgpack_pack_nil(pk)))) ||

        mp_pack_str(pk, "status") ||
        mp_pack_str(pk, ti_module_status_str(module)) ||

        mp_pack_str(pk, "scope") ||
        (module->scope_id
                ? mp_pack_str(pk, ti_scope_name_from_id(*module->scope_id))
                : msgpack_pack_nil(pk))
    );
}

ti_val_t * ti_module_as_mpval(ti_module_t * module, _Bool with_conf)
{
    ti_raw_t * raw;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_raw_t), sizeof(ti_raw_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (ti_module_info_to_pk(module, &pk, with_conf))
    {
        msgpack_sbuffer_destroy(&buffer);
        return NULL;
    }

    raw = (ti_raw_t *) buffer.data;
    ti_raw_init(raw, TI_VAL_MP, buffer.size);

    return (ti_val_t *) raw;
}

