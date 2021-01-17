/*
 * ti/proc.c
 */
#include <ti/module.t.h>
#include <ti/proc.h>
#include <ti.h>
#include <util/fx.h>


static void proc__alloc_buf(
        uv_handle_t * handle,
        size_t sugsz,
        uv_buf_t * uv_buf)
{
    ti_proc_t * proc = handle->data;
    buf_t * buf = &proc->buf;

    if (!buf->len && buf->cap != sugsz)
    {
        free(buf->data);
        buf->data = malloc(sugsz);
        if (buf->data == NULL)
        {
            log_error(EX_MEMORY_S);
            uv_buf->base = NULL;
            uv_buf->len = 0;
            return;
        }
        buf->cap = sugsz;
        buf->len = 0;
    }

    uv_buf->base = buf->data + buf->len;
    uv_buf->len = buf->cap - buf->len;
}

void proc__on_data(uv_stream_t * uvstream, ssize_t n, const uv_buf_t * uv_buf)
{
    ti_proc_t * proc = uvstream->data;
    buf_t * buf = &proc->buf;
    ti_pkg_t * pkg;
    size_t total_sz;

    if (n < 0)
    {
        LOGC(uv_strerror(n));
        return;
    }

    buf->len += n;
    if (buf->len < sizeof(ti_pkg_t))
        return;

    pkg = (ti_pkg_t *) buf->data;
    if (!ti_pkg_check(pkg))
    {
        log_error(
                "invalid package (type=%u invert=%u size=%u) "
                "from module `%s`",
                pkg->tp, pkg->ntp, pkg->n, proc->module->name->str);
        return;
    }

    total_sz = sizeof(ti_pkg_t) + pkg->n;

    if (buf->len < total_sz)
    {
        if (buf->cap < total_sz)
        {
            char * tmp = realloc(buf->data, total_sz);
            if (!tmp)
            {
                log_error(EX_MEMORY_S);
                return;
            }
            buf->data = tmp;
            buf->cap = total_sz;
        }
        return;
    }

    ti_module_on_pkg(proc->module, pkg);

    buf->len -= total_sz;
    if (buf->len > 0)
    {
        /* move data and call ti_stream_on_data() again */
        memmove(buf->data, buf->data + total_sz, buf->len);
        proc__on_data(uvstream, 0, uv_buf);
    }
}

static void proc__on_child_stdout_close(uv_handle_t * handle)
{
    ti_proc_t * proc = handle->data;

    /* clear buffer */
    free(proc->buf.data);
    buf_init(&proc->buf);

    ti_module_on_exit(proc->module);
}

static void proc__on_child_stdin_close(uv_handle_t * handle)
{
    ti_proc_t * proc = handle->data;
    uv_close((uv_handle_t *) &proc->child_stdout, proc__on_child_stdout_close);
}

static void proc__on_process_close(uv_process_t * process)
{
    ti_proc_t * proc = process->data;
    process->pid = 0;
    uv_close((uv_handle_t *) &proc->child_stdin, proc__on_child_stdin_close);
}

static void proc__on_exit(
        uv_process_t * process,
        int64_t exit_status,
        int term_signal)
{
    ti_proc_t * proc = process->data;

    if (exit_status)
    {
        log_error(
            "module `%s` has been stopped (%d) with exit status: %"PRId64,
            proc->module->name->str, term_signal, exit_status);
    }
    else
    {
        log_info(
            "module `%s` has been stopped (%d) with exit status: %"PRId64,
            proc->module->name->str, term_signal, exit_status);
    }

    uv_close((uv_handle_t *) process, (uv_close_cb) proc__on_process_close);
}

void ti_proc_init(ti_proc_t * proc, ti_module_t * module)
{
    memset(proc, 0, sizeof(ti_proc_t));

    proc->module = module;

    proc->options.stdio_count = 3;
    proc->options.stdio = proc->child_stdio;

    proc->child_stdio[0].flags = UV_CREATE_PIPE | UV_READABLE_PIPE;
    proc->child_stdio[0].data.stream = (uv_stream_t *) &proc->child_stdin;

    proc->child_stdio[1].flags = UV_CREATE_PIPE | UV_WRITABLE_PIPE;
    proc->child_stdio[1].data.stream = (uv_stream_t *) &proc->child_stdout;

    proc->child_stdio[2].flags = UV_INHERIT_FD;
    proc->child_stdio[2].data.fd = 2;

    proc->options.file = module->binary;
    proc->options.exit_cb = (uv_exit_cb) proc__on_exit;

    proc->child_stdin.data = proc;
    proc->child_stdout.data = proc;
    proc->process.data = proc;
}

/*
 * Return 0 on success or a libuv error when failed.
 */
int ti_proc_load(ti_proc_t * proc)
{
    int rc;

    if (!fx_file_exist(proc->options.file))
        return UV_ENOENT;  /* no such file or directory */

    rc = uv_pipe_init(ti.loop, &proc->child_stdin, 1);
    if (rc)
        goto fail0;

    rc = uv_pipe_init(ti.loop, &proc->child_stdout, 1);
    if (rc)
        goto fail1;

    rc = uv_spawn(ti.loop, &proc->process, &proc->options);
    if (rc)
        goto fail3;

    rc = uv_read_start(
            (uv_stream_t *) &proc->child_stdout,
            (uv_alloc_cb) proc__alloc_buf,
            (uv_read_cb) proc__on_data);
    if (rc)
        goto fail4;

    return 0;

fail4:
    if (proc->process.pid)
    {
        (void) uv_kill(proc->process.pid, SIGTERM);
        goto fail2;
    }
fail3:
    uv_close((uv_handle_t *) &proc->process, NULL);
fail2:
    uv_close((uv_handle_t *) &proc->child_stdout, NULL);
fail1:
    uv_close((uv_handle_t *) &proc->child_stdin, NULL);
fail0:
    return rc;
}


