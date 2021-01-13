#include <ti/ext/proc.h>
#include <ti/ext.h>
#include <ti.h>

typedef struct
{
    uv_process_t process;
    uv_process_options_t options;
    uv_stdio_container_t child_stdio[3];
    uv_pipe_t child_stdin;
    uv_pipe_t child_stdout;
} proc__t;

static void proc__alloc_buf(uv_handle_t * handle, size_t sugsz, uv_buf_t * buf)
{
    buf->base = malloc(sugsz);
    buf->len = sugsz;
}

void proc__on_data(uv_stream_t * uvstream, ssize_t n, const uv_buf_t * buf)
{
    LOGC("ON_DATA: %zd", n);
}

static void proc__on_exit(uv_process_t * process, int64_t exit_status, int term_signal)
{
    LOGC("ON_EXIT");
}

static void proc__on_stdout(uv_stream_t * server, int status)
{
    LOGC("ON STDOUT CONNECT");

//        uv_tcp_t *client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
//        uv_tcp_init(loop, client);
//        if (uv_accept(server, (uv_stream_t*) client) == 0) {
//            invoke_cgi_script(client);
//        }
//        else {
//            uv_close((uv_handle_t*) client, NULL);
//        }
}

static uv_stream_t * proc__child_stdin;


int ti_ext_proc_init(void)
{
    uv_process_t * process = calloc(sizeof(uv_process_t), 1);
    uv_process_options_t * options = calloc(sizeof(uv_process_options_t), 1);
    uv_stdio_container_t * child_stdio = calloc(sizeof(uv_stdio_container_t), 3);
    uv_pipe_t * child_stdin = malloc(sizeof(uv_pipe_t));
    uv_pipe_t * child_stdout = malloc(sizeof(uv_pipe_t));

    proc__child_stdin = (uv_stream_t *) child_stdin;

    uv_pipe_init(ti.loop, child_stdin, 1);
    uv_pipe_init(ti.loop, child_stdout, 1);
//    uv_pipe_bind(child_stdin, "requests.sock");


    options->stdio_count = 3;
    options->stdio = child_stdio;

//    uv_pipe_init()
//    uv_listen(child_stdout, 128, (uv_connection_cb) proc__on_stdout);

    child_stdio[0].flags = UV_CREATE_PIPE | UV_READABLE_PIPE;
    child_stdio[0].data.stream = (uv_stream_t *) child_stdin;

    child_stdio[1].flags = UV_CREATE_PIPE | UV_WRITABLE_PIPE;
    child_stdio[1].data.stream = (uv_stream_t *)child_stdout;

    child_stdio[2].flags = UV_INHERIT_FD;
    child_stdio[2].data.fd = 2;

    options->file = strdup("../ext/requests/requests");
    options->exit_cb = (uv_exit_cb) proc__on_exit;

    LOGC("SPAWN PROCESS");
    int rc = uv_spawn(ti.loop, process, options);
    if (rc)
        LOGC("rc: %s", uv_strerror(rc));

    uv_read_start((uv_stream_t *) child_stdout, (uv_alloc_cb) proc__alloc_buf, proc__on_data);

    ti_ext_t * ext = malloc(sizeof(ti_ext_t));

//    process->
    ext->cb = ti_ext_proc_cb;
    ext->data = NULL;
    ext->destroy_cb = NULL;
    smap_add(ti.extensions, "REQUESTS", ext);


    return 0;
}

static void proc__write_cb(uv_write_t * req, int status)
{
    LOGC("ON WRITE CALLBACK");
}

void ti_ext_proc_cb(ti_future_t * future)
{
    uv_buf_t wrbuf;
    ti_pkg_t * pkg = ti_pkg_new(0, 66, "blabla", 6);
    uv_write_t * req = malloc(sizeof(uv_write_t));
    LOGC("WRITE...");
    wrbuf = uv_buf_init((char *) pkg, sizeof(ti_pkg_t) + pkg->n);
    uv_write(req, proc__child_stdin, &wrbuf, 1, &proc__write_cb);
}
