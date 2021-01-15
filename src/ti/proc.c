/*
 * ti/proc.c
 */
#include <ti/module.t.h>
#include <ti/proc.h>
#include <ti.h>

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

void proc__on_data(uv_stream_t * uvstream, ssize_t n, const uv_buf_t * buf)
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
                "from module binary `%s`",
                pkg->tp, pkg->ntp, pkg->n, proc->module->binary->str);
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
        proc__on_data(uvstream, 0, buf);
    }
}

static void proc__on_exit(
        uv_process_t * process,
        int64_t exit_status,
        int term_signal)
{
    LOGC("ON_EXIT");
}


int ti_ext_proc_init(ti_proc_t * proc, ti_module_t * module)
{
    memset(proc, 0, sizeof(ti_proc_t));

    proc->module = module;

    if (uv_pipe_init(ti.loop, &proc->child_stdin, 1))
        goto fail0;

    if (uv_pipe_init(ti.loop, &proc->child_stdout, 1))
        goto fail1;

    proc->options.stdio_count = 3;
    proc->options.stdio = &proc->child_stdio;

    proc->child_stdio[0].flags = UV_CREATE_PIPE | UV_READABLE_PIPE;
    proc->child_stdio[0].data.stream = (uv_stream_t *) &proc->child_stdin;

    proc->child_stdio[1].flags = UV_CREATE_PIPE | UV_WRITABLE_PIPE;
    proc->child_stdio[1].data.stream = (uv_stream_t *) &proc->child_stdout;

    proc->child_stdio[2].flags = UV_INHERIT_FD;
    proc->child_stdio[2].data.fd = 2;

    proc->options.file = module->binary->str;
    proc->options.exit_cb = (uv_exit_cb) proc__on_exit;

    proc->child_stdout.data = proc;
    proc->process.data = proc;

    return 0;

fail1:
    uv_close((uv_handle_t *) &proc->child_stdin, NULL);
fail0:
    return -1;
}

int ti_proc_load(ti_proc_t * proc)
{
    int rc;

    rc = uv_spawn(ti.loop, &proc->process, &proc->options);
    if (rc)
        goto fail0;

    rc = uv_read_start(
            (uv_stream_t *) &proc->child_stdout,
            (uv_alloc_cb) proc__alloc_buf,
            (uv_read_cb) proc__on_data);
    if (rc)
        goto fail1;

    return 0;

fail1:
    uv_kill(proc->process->pid, SIGTERM);
fail0:
    proc->module->status = rc;
    return -1;
}

static void proc__write_cb(uv_write_t * req, int status)
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

int ti_proc_write_request(ti_proc_t * proc, uv_write_t * req, uv_buf_t * wrbuf)
{
    return uv_write(req, &proc->child_stdin, wrbuf, 1, &proc__write_cb);
}

