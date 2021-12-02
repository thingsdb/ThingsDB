/*
 * ti/module.c
 */
#include <ex.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/fn/fn.h>
#include <ti/future.h>
#include <ti/future.inline.h>
#include <ti/mod/github.h>
#include <ti/mod/manifest.h>
#include <ti/mod/work.h>
#include <ti/mod/work.t.h>
#include <ti/module.h>
#include <ti/names.h>
#include <ti/pkg.h>
#include <ti/proc.h>
#include <ti/proto.h>
#include <ti/proto.t.h>
#include <ti/query.h>
#include <ti/raw.inline.h>
#include <ti/raw.t.h>
#include <ti/scope.h>
#include <ti/thing.inline.h>
#include <ti/val.inline.h>
#include <ti/verror.h>
#include <util/fx.h>

#define MODULE__TOO_MANY_RESTARTS 3


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
    ti_vp_t vp;
    msgpack_sbuffer buffer;
    size_t alloc_sz = 1024;

    assert (ti_val_is_thing((ti_val_t *) thing));

    if (future->module->status)
    {
        ex_t e;
        ex_set(&e, EX_OPERATION,
                "module `%s` is not running (status: %s)",
                future->module->name->str,
                ti_module_status_str(future->module));
        ti_query_on_future_result(future, &e);
        return;
    }

    if (future->module->proc.process.pid == 0)
    {
        ex_t e;
        ex_set(&e, EX_OPERATION, "missing process ID for module `%s`",
                future->module->name->str);
        ti_query_on_future_result(future, &e);
        return;
    }

    if (future->module->flags & TI_MODULE_FLAG_WAIT_CONF)
    {
        ex_t e;
        ex_set(&e, EX_OPERATION,
                "module `%s` is not configured; "
                "if you keep this error, then please check the module status",
                future->module->name->str);
        ti_query_on_future_result(future, &e);
        return;
    }

    if (mp_sbuffer_alloc_init(&buffer, alloc_sz, sizeof(ti_pkg_t)))
        goto mem_error0;
    msgpack_packer_init(&vp.pk, &buffer, msgpack_sbuffer_write);

    /*
     * Add 1 to the `deep` value as we do not count the object itself towards
     * the `deep` value.
     */
    if (ti_thing_to_client_pk(thing, &vp, ti_future_deep(future) + 1))
        goto mem_error1;

    future->pkg = (ti_pkg_t *) buffer.data;
    pkg_init(future->pkg, 0, TI_PROTO_MODULE_REQ, buffer.size);

    log_debug("executing future for module `%s`", future->module->name->str);

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

ti_pkg_t * ti_module_conf_pkg(ti_val_t * val, ti_query_t * query)
{
    ti_pkg_t * pkg;
    ti_vp_t vp = {
            .query=query,
    };
    msgpack_sbuffer buffer;
    size_t alloc_sz = 1024;

    if (mp_sbuffer_alloc_init(&buffer, alloc_sz, sizeof(ti_pkg_t)))
        return NULL;
    msgpack_packer_init(&vp.pk, &buffer, msgpack_sbuffer_write);

    /*
     * Module configuration will be packed 2 levels deep. This is a fixed
     * setting and should be sufficient to configure a module.
     */
    if (ti_val_to_client_pk(val, &vp, 2))
    {
        msgpack_sbuffer_destroy(&buffer);
        return NULL;
    }

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_MODULE_CONF, buffer.size);

    return pkg;
}

_Bool ti_module_file_is_py(const char * file, size_t n)
{
    return file[n-3] == '.' &&
           file[n-2] == 'p' &&
           file[n-1] == 'y';
}

static int module__validate_source(const char * file, size_t file_n, ex_t * e)
{
    const char * pt = file;

    if (!file_n)
    {
        ex_set(e, EX_VALUE_ERROR,
                "source argument must not be an empty string"DOC_NEW_MODULE);
        return e->nr;
    }

    if (!strx_is_printablen((const char *) file, file_n))
    {
        ex_set(e, EX_VALUE_ERROR,
                "source argument contains illegal characters"DOC_NEW_MODULE);
        return e->nr;
    }


    if (*file == '/')
    {
        ex_set(e, EX_VALUE_ERROR,
                "source argument must not start with a `/`"DOC_NEW_MODULE);
        return e->nr;
    }

    for (size_t i = 1; i < file_n; ++i, ++pt)
    {
        if (*pt == '.' && (file[i] == '.' || file[i] == '/'))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "source argument must not contain `..` or `./` to specify "
                    "a relative path"DOC_NEW_MODULE);
            return e->nr;
        }
    }
    return 0;
}

ti_module_t* ti_module_create(
        const char * name,
        size_t name_n,
        const char * source,
        size_t source_n,
        uint64_t created_at,
        ti_pkg_t * conf_pkg,    /* may be NULL */
        uint64_t * scope_id,    /* may be NULL */
        ex_t * e)
{
    ti_module_t * module = calloc(1, sizeof(ti_module_t));
    if (!module)
    {
        ex_set_mem(e);
        return NULL;
    }

    module->ref = 1;
    module->status = TI_MODULE_STAT_NOT_INSTALLED;
    module->cb = (ti_module_cb) &module__cb;
    module->name = ti_names_get(name, name_n);
    module->created_at = created_at;
    module->futures = omap_create();
    module->path = fx_path_join_strn(
            ti.cfg->modules_path,
            strlen(ti.cfg->modules_path),
            name,
            name_n);
    module->orig = strndup(source, source_n);


    if (!module->name || !module->futures || !module->path || !module->orig)
        goto memerror;

    if (ti_mod_github_test(source, source_n))
    {
        module->source_type = TI_MODULE_SOURCE_GITHUB;
        module->source.github = ti_mod_github_create(source, source_n, e);
        if (!module->source.github)
            goto fail0;
    }
    else
    {
        if (module__validate_source(source, source_n, e))
            goto fail0;

        module->source_type = TI_MODULE_SOURCE_FILE;
        module->source.file = module->orig;
        module->manifest.is_py = ti_module_file_is_py(source, source_n);
    }

    /* The status might be changed for non-file based sources. */
    module->status = TI_MODULE_STAT_NOT_INSTALLED;

    /*
     * For non-file based sources we can try to lead the manifest and skip
     * the installation. Function `ti_module_set_file(..)` will set the status
     * to `TI_MODULE_STAT_NOT_LOADED` if successful.
     */
    if (module->source_type != TI_MODULE_SOURCE_FILE &&
        fx_is_dir(module->path) &&
        ti_mod_manifest_local(&module->manifest, module) == 0)
        (void) ti_module_set_file(
                module,
                module->manifest.main,
                strlen(module->manifest.main));

    switch (smap_add(ti.modules, module->name->str, module))
    {
    case SMAP_SUCCESS:
        break;
    case SMAP_ERR_ALLOC:
        goto memerror;
    case SMAP_ERR_EXIST:
        ex_set(e, EX_LOOKUP_ERROR,
                "module `%.*s` already exists", name_n, name);
        goto fail0;
    }

    module->scope_id = scope_id;
    module->conf_pkg = conf_pkg;

    return module;
memerror:
    ex_set_mem(e);
fail0:
    ti_module_drop(module);
    return NULL;
}

int ti_module_set_file(ti_module_t * module, const char * file, size_t n)
{
    char * str_file, ** args;
    args = malloc(sizeof(char*) * (module->manifest.is_py ? 3 : 2));
    str_file = fx_path_join_strn(module->path, strlen(module->path), file, n);

    if (!str_file || !args)
    {
        free(str_file);
        free(args);
        return -1;
    }

    free(module->file);
    free(module->args);

    module->file = str_file;
    module->args = args;
    module->fn = module->file + (strlen(module->file) - n);
    module->status = TI_MODULE_STAT_NOT_LOADED;

    if (module->manifest.is_py)
    {
        module->args[0] = ti.cfg->python_interpreter;
        module->args[1] = module->file;
        module->args[2] = NULL;
    }
    else
    {
        module->args[0] = module->file;
        module->args[1] = NULL;
    }

    ti_proc_init(&module->proc, module);
    return 0;
}

static void module__conf(ti_module_t * module)
{
    if (!module->conf_pkg)
    {
        module->flags &= ~TI_MODULE_FLAG_WAIT_CONF;
        log_debug("no configuration found for module `%s`", module->name->str);
        return;
    }

    module->status = module__write_conf(module);

    if (module->status == TI_MODULE_STAT_RUNNING)
        log_info("wrote configuration to module `%s`", module->name->str);
    else
        log_error(
                "failed to write configuration to module `%s` (%s): %s",
                module->name->str,
                module->file,
                ti_module_status_str(module));
}

static void module__install_finish(ti_mod_work_t * data)
{
    ti_module_t * module = data->module;

    /* move the manifest */
    ti_mod_manifest_clear(&module->manifest);
    module->manifest = data->manifest;
    ti_mod_manifest_init(&data->manifest);

    if (ti_module_set_file(
            module,
            module->manifest.main,
            strlen(module->manifest.main)))
    {
        log_error(EX_MEMORY_S);
        module->status = TI_MODULE_STAT_INSTALL_FAILED;
    }
    else
    {
        log_info("module `%s` (version: %s)",
                module->name->str,
                module->manifest.version);
        module->status = TI_MODULE_STAT_NOT_LOADED;
        ti_module_load(module);
    }
}

/*
 * Main thread
 */
static void module__py_requirements_finish(uv_work_t * work, int status)
{
    ti_mod_work_t * data = work->data;
    ti_module_t * module = data->module;

    if (status)
    {
        log_error(uv_strerror(status));
        goto warn;
    }

    goto done;

warn:
    log_warning("failed to install Python requirements for module `%s`",
            module->name->str);

done:
    /* store the manifest to disk */
    ti_mod_manifest_store(module->path, data->buf.data, data->buf.len);

    /* store the manifest to disk */
    module__install_finish(data);

    ti_mod_work_destroy(data);
    free(work);
}

/*
 * Work thread
 */
static void module__py_requirements_work(uv_work_t * work)
{
    ti_mod_work_t * data = work->data;
    ti_module_t * module = data->module;

    int rc = -1;
    char ch;
    FILE * fp;
    buf_t buf;
    buf_init(&buf);

    if (data->rtxt)
    {
        if (buf_append_fmt(
                &buf,
                "%s -m pip install -r \"%s/requirements.txt\" 2>&1;",
                ti.cfg->python_interpreter,
                module->path))
            goto fail0;
    }
    else
    {
        buf_append_fmt(&buf, "%s -m pip install", ti.cfg->python_interpreter);
        for (vec_each(module->manifest.requirements, char, str))
            if (buf_append_fmt(&buf, " \"%s\"", str))
                goto fail0;
    }

    if (buf_write(&buf, '\0'))
        goto fail0;

    fp = popen(buf.data, "r");
    if (!fp)
    {
        log_error("unable to open process (pip)");
        goto fail0;
    }

    while((ch = fgetc(fp)) != EOF)
        if (Logger.level == LOGGER_DEBUG)
            putchar(ch);

    if ((rc = pclose(fp)))
        log_error("failed to install Python requirements for module `%s` (%d)",
                module->name->str, rc);
    else
        log_info("successfully installed Python requirements for module `%s`",
                module->name->str, rc);

fail0:
    free(buf.data);
}

/*
 * Main thread
 */
static void module__py_requirements(uv_work_t * work)
{
    ti_mod_work_t * data = work->data;

    if (uv_queue_work(
            ti.loop,
            work,
            module__py_requirements_work,
            module__py_requirements_finish) == 0)
        return;  /* success */

    log_warning("failed to install Python requirements for module `%s`",
            data->module->name->str);

    /* store the manifest to disk */
    ti_mod_manifest_store(data->module->path, data->buf.data, data->buf.len);

    /* store the manifest to disk */
    module__install_finish(data);

    ti_mod_work_destroy(data);
    free(work);
}

static void module__download_finish(uv_work_t * work, int status)
{
    ti_mod_work_t * data = work->data;
    ti_module_t * module = data->module;

    if (module->source_err[0])
    {
        module->status = TI_MODULE_STAT_SOURCE_ERR;
        goto error;
    }

    if (status)
    {
        log_error(uv_strerror(status));
        if (module->status == TI_MODULE_STAT_INSTALLER_BUSY)
            module->status = TI_MODULE_STAT_INSTALL_FAILED;
        goto error;
    }

    if (data->rtxt || !vec_empty(module->manifest.requirements))
    {
        assert (data->manifest.is_py);
        module__py_requirements(work);
        return;
    }

    /* store the manifest to disk */
    ti_mod_manifest_store(module->path, data->buf.data, data->buf.len);

    /* store the manifest to disk */
    module__install_finish(data);
    goto done;

error:
    log_error("failed to install module: `%s` (%s)",
            data->module->name->str,
            ti_module_status_str(data->module));
done:
    ti_mod_work_destroy(data);
    free(work);
}

/*
 * Main thread
 */
static void module__download(uv_work_t * work)
{
    ti_mod_work_t * data = work->data;

    if (uv_queue_work(
            ti.loop,
            work,
            data->download_cb,
            module__download_finish) == 0)
        return;  /* success */

    data->module->status = TI_MODULE_STAT_INSTALL_FAILED;
    log_error("failed to install module: `%s` (%s)",
            data->module->name->str,
            ti_module_status_str(data->module));
    ti_mod_work_destroy(data);
    free(work);
}

static void module__manifest_finish(uv_work_t * work, int status)
{
    ti_mod_work_t * data = work->data;
    ti_module_t * module = data->module;

    ti_mod_manifest_init(&data->manifest);

    if (module->source_err[0])
    {
        if (ti_mod_manifest_local(&data->manifest, module) == 0)
        {
            log_warning(module->source_err);
            *module->source_err = '\0';
            goto done;
        }

        module->status = TI_MODULE_STAT_SOURCE_ERR;
        goto error;
    }

    if (status)
    {
        log_error(uv_strerror(status));
        if (module->status == TI_MODULE_STAT_INSTALLER_BUSY)
            module->status = TI_MODULE_STAT_INSTALL_FAILED;
        goto error;
    }

    if (module->status != TI_MODULE_STAT_INSTALLER_BUSY)
        goto error;

    if (module->ref > 1)  /* do nothing if this is the last reference */
    {
        if (ti_mod_manifest_read(
                &data->manifest,
                module->source_err,
                data->buf.data,
                data->buf.len))
        {
            if (ti_mod_manifest_local(&data->manifest, module) == 0)
            {
                log_warning(module->source_err);
                *module->source_err = '\0';
                goto done;
            }

            module->status = TI_MODULE_STAT_SOURCE_ERR;
            goto error;
        }

        if (ti_mod_manifest_skip_install(&data->manifest, module))
        {
            log_info("skip re-installing module `%s` (got version: %s)",
                    module->name->str,
                    data->manifest.version);
            goto done;  /* success, correct module is installed */
        }

        module__download(work);
        return;
    }

error:
    log_error("failed to install module: `%s` (%s)",
            module->name->str,
            ti_module_status_str(module));
    goto fail;
done:
    module__install_finish(data);
fail:
    ti_mod_work_destroy(data);
    free(work);
}

typedef struct
{
    void (*manifest_cb) (uv_work_t *);
    void (*download_cb) (uv_work_t *);
} module__install_t;

static void module__install(ti_module_t * module, module__install_t install)
{
    uv_work_t * work;
    ti_mod_work_t * data;
    int prev_status = module->status;

    module->status = TI_MODULE_STAT_INSTALLER_BUSY;
    module->source_err[0] = '\0';

    ti_incref(module);

    work = malloc(sizeof(uv_work_t));
    data = malloc(sizeof(ti_mod_work_t));
    if (!work || !data)
        goto fail;

    data->module = module;
    data->download_cb = install.download_cb;
    data->rtxt = false;
    work->data = data;

    buf_init(&data->buf);

    if (uv_queue_work(
            ti.loop,
            work,
            install.manifest_cb,
            module__manifest_finish) == 0)
        return;  /* success */

fail:
    free(work);
    free(data);
    ti_decref(module);
    module->status = prev_status;
    log_error("failed install module: `%s`", module->name->str);
}

void ti_module_load(ti_module_t * module)
{
    static const module__install_t module__gh_install = {
            .manifest_cb = ti_mod_github_manifest,
            .download_cb = ti_mod_github_download,
    };

    if (module->flags & TI_MODULE_FLAG_IN_USE)
    {
        log_debug(
                "module `%s` already loaded (PID %d)",
                module->name->str, module->proc.process.pid);
        return;
    }

    /* First check if the module is installed, if not we might need to install
     * the module first.
     */
    if (module->status == TI_MODULE_STAT_NOT_INSTALLED ||
        module->status == TI_MODULE_STAT_INSTALL_FAILED ||
        (module->flags & TI_MODULE_FLAG_UPDATE))
    {
        /* Only this flag will be set only once */
        module->flags &= ~TI_MODULE_FLAG_UPDATE;

        switch (module->source_type)
        {
        case TI_MODULE_SOURCE_FILE:
            if (fx_is_dir(module->path))
            {
                const char * file = module->source.file;
                if (ti_module_set_file(module, file, strlen(file)))
                {
                    log_error(EX_MEMORY_S);
                    return;
                }
            }
            else
                return;  /* not installed */
            /* status has been changed to TI_MODULE_STAT_NOT_LOADED */
            break;
        case TI_MODULE_SOURCE_GITHUB:
            {
                module__install(module, module__gh_install);
                return;
            }
        }
    }

    if (module->status != TI_MODULE_STAT_NOT_LOADED)
    {
        log_debug("skip loading module `%s` (status: %s)",
                module->name->str, ti_module_status_str(module));
        return;
    }

    module->flags |= TI_MODULE_FLAG_WAIT_CONF;
    module->status = ti_proc_load(&module->proc);

    if (module->status == TI_MODULE_STAT_RUNNING)
    {
        log_info(
                "loaded module `%s` (%s)",
                module->name->str,
                module->file);
        module__conf(module);
    }
    else
    {
        log_error(
                "failed to start module `%s` (%s): %s",
                module->name->str,
                module->file,
                ti_module_status_str(module));
    }
}

void ti_module_restart(ti_module_t * module)
{
    log_info("restarting module `%s`...", module->name->str);
    if (module->flags & TI_MODULE_FLAG_IN_USE)
    {
        module->flags |= TI_MODULE_FLAG_RESTARTING;
        (void) ti_module_stop(module);
    }
    else
    {
        ti_module_load(module);
    }
}

typedef struct
{
    ti_module_t * module;
    void * data;
    size_t n;
} module__wait_deploy_t;

/*
 * Ready is not the same is running, a module is ready when not installing,
 * loading or restarting.
 */
_Bool ti_module_is_ready(ti_module_t * module)
{
    return ((~module->flags & TI_MODULE_FLAG_RESTARTING) && (
          module->status == TI_MODULE_STAT_RUNNING ||
          module->status == TI_MODULE_STAT_TOO_MANY_RESTARTS ||
          module->status == TI_MODULE_STAT_PY_INTERPRETER_NOT_FOUND ||
          module->status == TI_MODULE_STAT_CONFIGURATION_ERR ||
          module->status == TI_MODULE_STAT_SOURCE_ERR ||
          module->status == TI_MODULE_STAT_INSTALL_FAILED)
    );
}

static inline _Bool module__may_deploy(ti_module_t * module)
{
    return !(
        module->status == TI_MODULE_STAT_INSTALLER_BUSY ||
        module->status == TI_MODULE_STAT_STOPPING ||
        (module->flags & TI_MODULE_FLAG_RESTARTING));
}

static void module__wait_close_cb(uv_timer_t * timer)
{
    module__wait_deploy_t * wdeploy = timer->data;
    ti_module_drop(wdeploy->module);
    free(wdeploy->data);
    free(wdeploy);
    free(timer);
}

static void module__wait_cb(uv_timer_t * timer)
{
    module__wait_deploy_t * wdeploy = timer->data;
    ti_module_t * module = wdeploy->module;

    if (module__may_deploy(module))
    {
        (void) ti_module_deploy(module, wdeploy->data, wdeploy->n);

        module->wait_deploy = NULL;
        uv_timer_stop(timer);
        uv_close((uv_handle_t *) timer, (uv_close_cb) module__wait_close_cb);
    }
}

static int module__wait_deploy(
        ti_module_t * module,
        const void * data,
        size_t n)
{
    module__wait_deploy_t * wdeploy;
    void * mdata = NULL;

    log_debug("delay deployment for module `%s`", module->name->str);

    if (data)
    {
        mdata = malloc(sizeof(n));
        if (!mdata)
            goto fail0;
        memcpy(mdata, data, n);
    }

    if (module->wait_deploy)
    {
        wdeploy = module->wait_deploy->data;
        ti_module_drop(wdeploy->module);
        free(wdeploy->data);
    }
    else
    {
        wdeploy = malloc(sizeof(module__wait_deploy_t));
        if (!wdeploy)
            goto fail0;

        module->wait_deploy = malloc(sizeof(uv_timer_t));
        if (!module->wait_deploy)
            goto fail1;

        module->wait_deploy->data = wdeploy;

        if (uv_timer_init(ti.loop, module->wait_deploy))
            goto fail2;

        if (uv_timer_start(module->wait_deploy, module__wait_cb, 1000, 1000))
            goto fail3;
    }

    wdeploy->data = mdata;
    wdeploy->n = n;
    wdeploy->module = module;
    ti_incref(module);

    return 0;
fail3:
    uv_close((uv_handle_t *) module->wait_deploy, (uv_close_cb) free);
    goto fail1;
fail2:
    free(module->wait_deploy);
fail1:
    free(wdeploy);
fail0:
    free(mdata);
    return -1;
}

int ti_module_deploy(ti_module_t * module, const void * data, size_t n)
{
    if (!module__may_deploy(module))
        return module__wait_deploy(module, data, n);

    switch (module->source_type)
    {
    case TI_MODULE_SOURCE_FILE:
        if (data || ti_module_write(module, data, n))
            return -1;
        break;
    case TI_MODULE_SOURCE_GITHUB:
        if (data)
        {
            ex_t e = {0};
            ti_mod_github_t * github = ti_mod_github_create(data, n, &e);
            char * orig = strndup(data, n);

            if (!github || !orig)
            {
                if (e.nr == 0)
                    ex_set_mem(&e);
                log_error(e.msg);
                free(orig);
                ti_mod_github_destroy(github);
                return -1;
            }

            ti_mod_github_destroy(module->source.github);
            free(module->orig);
            module->orig = orig;
            module->source.github = github;
        }
        module->flags |= TI_MODULE_FLAG_UPDATE;
        break;
    }

    if (!(ti_flag_test(TI_FLAG_STARTING)))
        ti_module_restart(module);

    return 0;
}

void ti_module_update_conf(ti_module_t * module)
{
    if (module->flags & TI_MODULE_FLAG_IN_USE)
        module__conf(module);
    else
        ti_module_load(module);
}

void ti_module_on_exit(ti_module_t * module)
{
    /* First cancel all open futures */
    ti_module_cancel_futures(module);

    module->flags &= ~TI_MODULE_FLAG_IN_USE;

    if (module->flags & TI_MODULE_FLAG_DESTROY)
    {
        /* On destroy flag, just drop the module */
        ti_module_drop(module);
        return;
    }

    if (ti_flag_test(TI_FLAG_SIGNAL))
    {
        /* Catch a signal before restart */
        module->status = TI_MODULE_STAT_NOT_LOADED;
        return;
    }

    if (module->flags & TI_MODULE_FLAG_RESTARTING)
    {
        /* Catch a restart before the stopping status */
        module->restarts = 0;
        module->flags &= ~TI_MODULE_FLAG_RESTARTING;
        goto restart;
    }

    if (module->status == TI_MODULE_STAT_STOPPING)
    {
        /* Apparently we just want to stop the module  */
        module->status = TI_MODULE_STAT_NOT_LOADED;
        return;
    }

    if (module->status == TI_MODULE_STAT_RUNNING)
    {
        /* Unexpected exit, maybe we can restart the module */
        if (++module->restarts > MODULE__TOO_MANY_RESTARTS)
        {
            log_error(
                    "module `%s` has been restarted too many (>%d) times",
                    module->name->str,
                    MODULE__TOO_MANY_RESTARTS);
            module->status = TI_MODULE_STAT_TOO_MANY_RESTARTS;
            return;
        }
        goto restart;
    }

    log_warning("module `%s` got an unexpected status on exit: %s",
            module->name->str,
            ti_module_status_str(module));
    return;

restart:
    module->status = TI_MODULE_STAT_NOT_LOADED;
    ti_module_load(module);
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
        ti_module_drop(module);
}

void module__stop_cb(uv_async_t * task)
{
    ti_module_t * module = task->data;
    ti_module_stop_and_destroy(module);
    uv_close((uv_handle_t *) task, (uv_close_cb) free);
}

void ti_module_del(ti_module_t * module, _Bool delete_files)
{
    uv_async_t * task;

    if (delete_files)
        /* set the DELETE flag as the module needs to be removed */
        module->flags |= TI_MODULE_FLAG_DEL_FILES;

    (void) smap_pop(ti.modules, module->name->str);

    task = malloc(sizeof(uv_async_t));
    if (task && uv_async_init(ti.loop, task, module__stop_cb) == 0)
    {
        task->data = module;
        (void) uv_async_send(task);
    }
}

void ti_module_cancel_futures(ti_module_t * module)
{
    omap_clear(module->futures, (omap_destroy_cb) ti_future_cancel);
}

static void module__on_res(ti_future_t * future, ti_pkg_t * pkg)
{
    ex_t e = {0};
    ti_val_t * val;

    if (ti_future_should_load(future))
    {
        mp_unp_t up;
        ti_vup_t vup = {
                .isclient = true,
                .collection = future->query->collection,
                .up = &up,
        };
        mp_unp_init(&up, pkg->data, pkg->n);
        val = ti_val_from_vup_e(&vup, &e);
    }
    else if (!mp_is_valid(pkg->data, pkg->n))
    {
        ex_set(&e, EX_BAD_DATA,
                "got invalid or corrupt MsgPack data from module: `%s`",
                future->module->name->str);
        val = NULL;
    }
    else
        val = (ti_val_t *) ti_mp_create(pkg->data, pkg->n);

    if (!val)
    {
        if (e.nr == 0)
            ex_set_mem(&e);
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
    ti_future_t * future;

    switch(pkg->tp)
    {
    case TI_PROTO_MODULE_CONF_OK:
        log_info("module `%s` is successfully configured", module->name->str);
        module->status &= ~TI_MODULE_STAT_CONFIGURATION_ERR;
        module->flags &= ~TI_MODULE_FLAG_WAIT_CONF;
        return;
    case TI_PROTO_MODULE_CONF_ERR:
        log_info("failed to configure module `%s`", module->name->str);
        module->status = TI_MODULE_STAT_CONFIGURATION_ERR;
        module->flags &= ~TI_MODULE_FLAG_WAIT_CONF;
        return;
    }

    future = omap_rm(module->futures, pkg->id);
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
    switch ((ti_module_stat_t) module->status)
    {
    case TI_MODULE_STAT_RUNNING:
        return "running";
    case TI_MODULE_STAT_NOT_INSTALLED:
        return "module not installed";
    case TI_MODULE_STAT_INSTALLER_BUSY:
        return "installing module...";
    case TI_MODULE_STAT_NOT_LOADED:
        return "module not loaded";
    case TI_MODULE_STAT_STOPPING:
        return "stopping module...";
    case TI_MODULE_STAT_TOO_MANY_RESTARTS:
        return "too many restarts detected; most likely the module is broken";
    case TI_MODULE_STAT_PY_INTERPRETER_NOT_FOUND:
        return "the Python interpreter is not found on this node"
                DOC_NODE_INFO""DOC_CONFIGURATION;
    case TI_MODULE_STAT_CONFIGURATION_ERR:
        return "configuration error; "
               "use `set_module_conf(..)` to update the module configuration";
    case TI_MODULE_STAT_INSTALL_FAILED:
        return "module installation has failed";
    case TI_MODULE_STAT_SOURCE_ERR:
        return module->source_err;
    }
    return uv_strerror(module->status);
}

static int module__info_to_vp(ti_module_t * module, ti_vp_t * vp, int flags)
{
    msgpack_packer * pk = &vp->pk;
    ti_mod_manifest_t * manifest = &module->manifest;
    size_t defaults_n = \
            (manifest->defaults ? manifest->defaults->n : 0) + \
            !!manifest->deep + \
            !!manifest->load;
    size_t sz = 4 + \
            !!(module->file) + \
            !!(flags & TI_MODULE_FLAG_WITH_CONF) + \
            !!(flags & TI_MODULE_FLAG_WITH_TASKS) + \
            !!(flags & TI_MODULE_FLAG_WITH_RESTARTS) + \
            ((module->source_type == TI_MODULE_SOURCE_GITHUB) ? 4 : 0) + \
            !!(manifest->version) + \
            !!(manifest->doc) + \
            !!(manifest->exposes) + \
            !!defaults_n;

    if (msgpack_pack_map(pk, sz) ||

        mp_pack_str(pk, "name") ||
        mp_pack_strn(pk, module->name->str, module->name->n) ||

        mp_pack_str(pk, "created_at") ||
        msgpack_pack_uint64(pk, module->created_at) ||

        mp_pack_str(pk, "status") ||
        mp_pack_str(pk, ti_module_status_str(module)) ||

        mp_pack_str(pk, "scope") ||
        (module->scope_id
                ? mp_pack_str(pk, ti_scope_name_from_id(*module->scope_id))
                : msgpack_pack_nil(pk)) ||

        (module->file && (
            mp_pack_str(pk, "file") ||
            mp_pack_str(pk, module->file))) ||

        ((flags & TI_MODULE_FLAG_WITH_CONF) && (
                mp_pack_str(pk, "conf") ||
                (module->conf_pkg
                        ? mp_pack_append(
                            pk,
                            module->conf_pkg + sizeof(ti_pkg_t),
                            module->conf_pkg->n)
                        : msgpack_pack_nil(pk)))) ||

        ((flags & TI_MODULE_FLAG_WITH_TASKS) && (
                mp_pack_str(pk, "tasks") ||
                msgpack_pack_uint64(pk, module->futures->n))) ||

        ((flags & TI_MODULE_FLAG_WITH_RESTARTS) && (
                mp_pack_str(pk, "restarts") ||
                msgpack_pack_uint16(pk, module->restarts))) ||

        ((module->source_type == TI_MODULE_SOURCE_GITHUB) && (
                mp_pack_str(pk, "github_owner") ||
                mp_pack_str(pk, module->source.github->owner) ||

                mp_pack_str(pk, "github_repo") ||
                mp_pack_str(pk, module->source.github->repo) ||

                mp_pack_str(pk, "github_with_token") ||
                (module->source.github->token_header ?
                        msgpack_pack_true(pk) :
                        msgpack_pack_false(pk)) ||

                mp_pack_str(pk, "github_ref") ||
                (module->source.github->ref ?
                        mp_pack_str(pk, module->source.github->ref) :
                        mp_pack_str(pk, "default")))) ||

        (manifest->version && (
                mp_pack_str(pk, "version") ||
                mp_pack_str(pk, manifest->version))) ||

        (manifest->doc && (
                mp_pack_str(pk, "doc") ||
                mp_pack_str(pk, manifest->doc))))
        return -1;

    if (defaults_n)
    {
        if (mp_pack_str(pk, "defaults") ||
            msgpack_pack_map(pk, defaults_n))
            return -1;

        if (manifest->deep && (
            mp_pack_str(pk, "deep") ||
            msgpack_pack_uint8(pk, *manifest->deep)))
            return -1;

        if (manifest->load && (
            mp_pack_str(pk, "load") ||
            mp_pack_bool(pk, *manifest->load)))
            return -1;

        if (manifest->defaults)
            for (vec_each(manifest->defaults, ti_item_t, item))
                if (mp_pack_strn(pk, item->key->data, item->key->n) ||
                    ti_val_to_client_pk(item->val, vp, TI_MAX_DEEP_HINT))
                    return -1;
    }

    if (manifest->exposes)
    {
        char namebuf[TI_NAME_MAX];
        smap_t * exposes = manifest->exposes;
        if (mp_pack_str(pk, "exposes") ||
            msgpack_pack_map(pk, exposes->n) ||
            smap_items(
                    exposes,
                    namebuf,
                    (smap_item_cb) ti_mod_expose_info_to_vp,
                    vp))
            return -1;
    }

    return 0;
}

ti_val_t * ti_module_as_mpval(ti_module_t * module, int flags)
{
    ti_raw_t * raw;
    msgpack_sbuffer buffer;
    ti_vp_t vp = {
            .query = NULL
    };

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_raw_t), sizeof(ti_raw_t));
    msgpack_packer_init(&vp.pk, &buffer, msgpack_sbuffer_write);

    if (module__info_to_vp(module, &vp, flags))
    {
        msgpack_sbuffer_destroy(&buffer);
        return NULL;
    }

    raw = (ti_raw_t *) buffer.data;
    ti_raw_init(raw, TI_VAL_MPDATA, buffer.size);

    return (ti_val_t *) raw;
}

int ti_module_write(ti_module_t * module, const void * data, size_t n)
{
    if (ti_module_is_py(module))
        return fx_write(module->file, data, n);

    if (fx_file_exist(module->file) && unlink(module->file))
    {
        log_errno_file("cannot delete file", errno, module->file);
        return -1;
    }

    if (fx_write(module->file, data, n))
        return -1;

    if (chmod(module->file, S_IRWXU))
    {
        log_errno_file("cannot make file executable", errno, module->file);
        return -1;
    }

    return 0;
}

int ti_module_read_args(
        ti_module_t * module,
        ti_thing_t * thing,
        _Bool * load,
        uint8_t * deep,
        ex_t * e)
{
    ti_name_t * deep_name = (ti_name_t *) ti_val_borrow_deep_name();
    ti_name_t * load_name = (ti_name_t *) ti_val_borrow_load_name();
    ti_val_t * deep_val = ti_thing_val_weak_by_name(thing, deep_name);
    ti_val_t * load_val = ti_thing_val_weak_by_name(thing, load_name);

    if (!deep_val)
        *deep = module->manifest.deep
            ? *module->manifest.deep
            : TI_MODULE_DEFAULT_DEEP;
    else if (ti_deep_from_val(deep_val, deep, e))
        return e->nr;

    *load = load_val
            ? ti_val_as_bool(load_val)
            : module->manifest.load
            ? *module->manifest.load
            : TI_MODULE_DEFAULT_LOAD;

    return 0;
}

int ti_module_set_defaults(ti_thing_t ** thing, vec_t * defaults)
{
    if (!defaults || !defaults->n)
        return 0;

    if ((ti_thing_is_instance(*thing) || (*thing)->ref > 1) &&
        ti_thing_copy(thing, 1))
        return -1;

    for (vec_each(defaults, ti_item_t, item))
    {
        if (ti_thing_has_key(*thing, item->key))
            continue;

        if (ti_thing_o_add(*thing, item->key, item->val))
            return -1;

        ti_incref(item->key);
        ti_incref(item->val);
    }
    return 0;
}

int ti_module_call(
        ti_module_t * module,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e)
{
    assert (query->rval == NULL);

    const int nargs = fn_get_nargs(nd);
    _Bool load = false;
    uint8_t deep = 1;
    cleri_children_t * child = nd->children;
    ti_future_t * future;

    if (ti.futures_count >= TI_MAX_FUTURE_COUNT)
    {
        ex_set(e, EX_MAX_QUOTA,
                "maximum number of active futures (%u) is reached",
                TI_MAX_FUTURE_COUNT);
        return e->nr;
    }

    if (module->scope_id && *module->scope_id != ti_query_scope_id(query))
    {
        ex_set(e, EX_FORBIDDEN,
                "module `%s` is restricted to scope `%s`",
                module->name->str,
                ti_scope_name_from_id(*module->scope_id));
        return e->nr;
    }

    if (nargs < 1)
    {
        ex_set(e, EX_NUM_ARGUMENTS,
                "modules must be called using at least 1 argument "
                "but 0 were given"DOC_MODULES);
        return e->nr;
    }

    ti_incref(module);  /* take a reference to module */

    if (ti_do_statement(query, child->node, e))
        goto fail0;

    if (!ti_val_is_thing(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "expecting the first module argument to be of "
                "type `"TI_VAL_THING_S"` but got type `%s` instead"DOC_MODULES,
                ti_val_str(query->rval));
        goto fail0;
    }

    if (ti_module_read_args(
                module,
                (ti_thing_t *) query->rval,
                &load,
                &deep,
                e) ||
        ti_module_set_defaults(
                (ti_thing_t **) &query->rval,
                module->manifest.defaults))
        goto fail0;

    future = ti_future_create(query, module, nargs, deep, load);
    if (!future)
        goto fail0;

    VEC_push(future->args, query->rval);
    query->rval = NULL;

    while ((child = child->next) && (child = child->next))
    {
        if (ti_do_statement(query, child->node, e))
            goto fail2;

        VEC_push(future->args, query->rval);
        query->rval = NULL;
    }

    if (ti_future_register(future))
        goto fail2;

    query->rval = (ti_val_t *) future;
    ti_decref(module);  /* the future is guaranteed to have at least
                           one reference */
    return e->nr;

fail2:
    ti_val_unsafe_drop((ti_val_t *) future);
fail0:
    if (e->nr == 0)
        ex_set_mem(e);
    ti_module_drop(module);
    return e->nr;
}

void ti_module_set_source_err(ti_module_t * module, const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    (void) vsnprintf(module->source_err, TI_MODULE_MAX_ERR, fmt, args);
    va_end(args);
}

void ti_module_destroy(ti_module_t * module)
{
    if (!module)
        return;

    if (module->wait_deploy)
    {
        uv_timer_stop(module->wait_deploy);
        uv_close(
                (uv_handle_t *) module->wait_deploy,
                (uv_close_cb) module__wait_close_cb);
    }

    omap_destroy(module->futures, (omap_destroy_cb) ti_future_cancel);

    if ((module->source_type != TI_MODULE_SOURCE_FILE) &&
        (module->flags & TI_MODULE_FLAG_DEL_FILES) &&
        module->path && fx_rmdir(module->path))
        log_warning("cannot remove directory: `%s`", module->path);

    ti_val_drop((ti_val_t *) module->name);
    free(module->orig);
    free(module->path);
    free(module->file);
    free(module->args);
    free(module->conf_pkg);
    free(module->scope_id);

    switch (module->source_type)
    {
    case TI_MODULE_SOURCE_FILE:
        break;
    case TI_MODULE_SOURCE_GITHUB:
        ti_mod_github_destroy(module->source.github);
    }

    ti_mod_manifest_clear(&module->manifest);
    free(module);
}
