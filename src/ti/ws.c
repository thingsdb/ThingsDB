/*
 * ti/ws.c
 *
 * WebSockets support.
 */
#include <libwebsockets.h>
#include <ti.h>
#include <ti/stream.t.h>
#include <ti/user.t.h>
#include <ti/write.h>
#include <ti/ws.h>
#include <util/buf.h>
#include <util/logger.h>
#include <util/queue.h>
#include <util/fx.h>

static struct lws_context * ws__context;
const size_t ws__mf = LWS_SS_MTU-LWS_PRE;

static int ws__callback_established(struct lws * wsi, ti_ws_t * pss)
{
    const size_t sugsz = 8192;

    pss->queue = queue_new(16);
    if (!pss->queue)
        goto fail0;

    pss->stream = ti_stream_create(TI_STREAM_WS_IN_CLIENT, &ti_clients_pkg_cb);
    if (!pss->stream)
        goto fail1;

    pss->stream->buf = malloc(sugsz);
    if (!pss->stream->buf)
        goto fail2;

    pss->stream->with.ws = pss;
    pss->wsi = wsi;
    pss->f = 0;
    pss->n = 0;
    pss->nf = 0;

    return 0;

fail2:
    ti_stream_close(pss->stream);
fail1:
    queue_destroy(pss->queue, NULL);
    pss->queue = NULL;
fail0:
    log_error(EX_MEMORY_S);
    return -1;
}

static inline void ws__done(ti_ws_t * pss, ti_write_t * req, ex_enum status)
{
    (void) queue_shift(pss->queue);
    pss->f = 0;  /* reset to frame 0 */
    req->cb_(req, status);
}

static int ws__callback_server_writable(struct lws * wsi, ti_ws_t * pss)
{
    unsigned char mtubuff[LWS_SS_MTU];
    unsigned char * out = mtubuff + LWS_PRE;
    unsigned char * pt;
    int flags, m, is_end;
    size_t len;
    ti_write_t * req = queue_first(pss->queue);
    if (!req)
        return 0;  /* nothing to write */

    if (pss->f == 0)
    {
        /* notice we allowed for LWS_PRE in the payload already */
        size_t n = sizeof(ti_pkg_t) + req->pkg->n;

        pss->f = 1;
        pss->n = n;
        pss->nf = (n-1)/ws__mf+1;  /* calculate the number of frames */
    }

    /* set write flags for frame */
    is_end = pss->f==pss->nf;
    flags = lws_write_ws_flags(LWS_WRITE_BINARY, pss->f==1, is_end);

    /* calculate how much data must be send; if this is the last frame we use
     * the module, with the exception when the data is exact */
    len = is_end ? pss->n % ws__mf : ws__mf;
    len = len ? len : ws__mf;

    /* pointer to the data */
    pt = (unsigned char *) req->pkg;

    /* copy data to the buffer */
    memcpy(out, pt + (pss->f-1)*ws__mf, len);

    /* write to websocket */
    m = lws_write(wsi, out, len, flags);

    if (m < (int) len)
    {
        log_error("ERROR %d; writing to WebSocket", m);
        ws__done(pss, req, EX_WRITE_UV);
        return -1;
    }

    if (is_end)
        ws__done(pss, req, 0);
    else
        pss->f++;  /* next frame */

    /* request next callback, even when finished as a new package might exist
     * in the queue */
    lws_callback_on_writable(wsi);
    return 0;
}

static int ws__callback_receive(
        struct lws * wsi,
        ti_ws_t * pss,
        void * in,
        size_t len)
{
    ti_stream_t * stream = pss->stream;
    if (lws_is_first_fragment(wsi))
        stream->n = 0;

    if (len > stream->sz - stream->n)
    {
        size_t sz = stream->n + len;
        char * nbuf = realloc(stream->buf, sz);
        if (!nbuf)
        {
            log_error(EX_MEMORY_S);
            return -1;
        }
        stream->buf = nbuf;
        stream->sz = sz;
    }

    memcpy(stream->buf + stream->n, in, len);
    stream->n += len;

    if (lws_is_final_fragment(wsi))
    {
        ti_pkg_t * pkg;
        if (stream->n < sizeof(ti_pkg_t))
        {
            log_error(
                    "invalid package (header too small) from `%s`, "
                    "closing connection",
                    ti_stream_name(stream));
            ti_stream_close(stream);
            return -1;
        }
        pkg = (ti_pkg_t *) stream->buf;
        stream->n = 0;  /* reset buffer */
        if (!ti_pkg_check(pkg))
        {
            log_error(
                    "invalid package (type=%u invert=%u size=%u) from `%s`, "
                    "closing connection",
                    pkg->tp, pkg->ntp, pkg->n, ti_stream_name(stream));
            ti_stream_close(stream);
            return -1;
        }
        stream->pkg_cb(stream, pkg);
    }
    lws_callback_on_writable(wsi);
    return 0;
}

struct per_vhost_data__minimal {
    struct lws_context *context;
    struct lws_vhost *vhost;
    const struct lws_protocols *protocol;
};

static void ws__kill_req(ti_write_t * req)
{
    req->cb_(req, EX_WRITE_UV);
}

/* Callback function for WebSocket server messages */
int ws__callback(
        struct lws * wsi,
        enum lws_callback_reasons reason,
        void * user,
        void * in,
        size_t len)
{
    ti_ws_t * pss = (ti_ws_t *) user;

    switch (reason)
    {
    case LWS_CALLBACK_PROTOCOL_INIT:
        break;
    case LWS_CALLBACK_ESTABLISHED:
        return ws__callback_established(wsi, pss);
    case LWS_CALLBACK_SERVER_WRITEABLE:
        return ws__callback_server_writable(wsi, pss);
    case LWS_CALLBACK_RECEIVE:
        return ws__callback_receive(wsi, pss, in, len);
    case LWS_CALLBACK_CLOSED:
        if (pss->stream)
        {
            queue_destroy(pss->queue, (queue_destroy_cb) ws__kill_req);
            ti_stream_close(pss->stream);
        }
        break;
    default:
        break;
    }
    return 0;
}

static struct lws_protocols ws__protocols[] = {
    {
        .name                  = "thingsdb-protocol",   /* Protocol name*/
        .callback              = ws__callback,          /* Protocol callback */
        .per_session_data_size = sizeof(ti_ws_t),       /* Protocol callback 'userdata' size */
        .rx_buffer_size        = 0,                     /* Receive buffer size (0 = no restriction) */
        .id                    = 0,                     /* Protocol Id (version) (optional) */
        .user                  = NULL,                  /* 'User data' ptr, to access in 'protocol callback */
        .tx_packet_size        = 0                      /* Transmission buffer size restriction (0 = no restriction) */
    },
    { NULL, NULL, 0, 0, 0, NULL, 0 } // Protocol list ends with NULL
};

void ws__log(int level, const char * line)
{
    switch(level)
    {
    case LLL_ERR:       log_line(LOGGER_INFO, line);        break;
    case LLL_WARN:      log_line(LOGGER_WARNING, line);     break;
    case LLL_DEBUG:     log_line(LOGGER_DEBUG, line);       break;
    case LLL_USER:
    case LLL_NOTICE:    log_line(LOGGER_INFO, line);        break;
    }
}

static int ws__with_certificates(void)
{
    if (ti.cfg->ws_cert_file && !ti.cfg->ws_key_file)
    {
        log_warning("got a certificate file but no private key file");
        return 0;
    }
    if (!ti.cfg->ws_cert_file && ti.cfg->ws_key_file)
    {
        log_warning("got a private key file but no certificate file");
        return 0;
    }

    if (ti.cfg->ws_cert_file && ti.cfg->ws_key_file)
    {
        if (!fx_file_exist(ti.cfg->ws_cert_file))
        {
            log_error("file does not exist: %s", ti.cfg->ws_cert_file);
            return 0;
        }

        if (!fx_file_exist(ti.cfg->ws_key_file))
        {
            log_error("file does not exist: %s", ti.cfg->ws_key_file);
            return 0;
        }

        if (!fx_starts_with(
                ti.cfg->ws_cert_file,
                "-----BEGIN CERTIFICATE-----\n"))
        {
            log_error(
                    "not a certificate file: `%s`; "
                    "content of this file must start with: "
                    "-----BEGIN CERTIFICATE-----",
                    ti.cfg->ws_cert_file);
            return 0;
        }

        if (!fx_starts_with(
                ti.cfg->ws_key_file,
                "-----BEGIN PRIVATE KEY-----\n"))
        {
            log_error(
                    "not a private key file: `%s`; "
                    "content of this file must start with: "
                    "-----BEGIN PRIVATE KEY-----",
                    ti.cfg->ws_key_file);
            return 0;
        }

        return 1;  /* success sanity check */
    }
    return 0;
}

int ti_ws_init()
{
    void *foreign_loops[1];
    struct lws_context_creation_info info;
    int logs = 0;

    switch (Logger.level)
    {
    /* for LLL_ verbosity above NOTICE to be built into lws,
     * lws must have been configured and built with
     * -DCMAKE_BUILD_TYPE=DEBUG instead of =RELEASE */
    /* | LLL_INFO */ /* | LLL_PARSER */ /* | LLL_HEADER */
    /* | LLL_EXT */ /* | LLL_CLIENT */ /* | LLL_LATENCY */
    /* | LLL_DEBUG */
    case LOGGER_DEBUG:
        logs |= LLL_DEBUG;
        /*fall through*/
    case LOGGER_INFO:
        logs |= LLL_USER | LLL_NOTICE;
        /*fall through*/
    case LOGGER_WARNING:
        logs |= LLL_WARN;
        /*fall through*/
    case LOGGER_ERROR: logs |= LLL_ERR;
    }

    foreign_loops[0] = ti.loop;
    memset(&info, 0, sizeof info); /* otherwise uninitialized garbage */

    info.port = (int) ti.cfg->ws_port;
    if (info.port == 0)
        return 0;

    info.protocols = ws__protocols;
    info.foreign_loops = foreign_loops;
    info.options = \
        LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE |
        LWS_SERVER_OPTION_LIBUV;

#if defined(LWS_WITH_TLS)
    if (ws__with_certificates())
    {
        info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
        info.ssl_cert_filepath = ti.cfg->ws_cert_file;
        info.ssl_private_key_filepath = ti.cfg->ws_key_file;
    }
#endif

    log_info(
            "start listening for WebSocket%s connections on TCP port %d",
            (info.options & LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT)
            ? " (Secure: TLS/SSL)"
            : "",
            info.port);

    lws_set_log_level(logs, &ws__log);

    ws__context = lws_create_context(&info);
    if (!ws__context)
    {
        log_error("lws init failed");
        return -1;
    }
    return 0;
}

int ti_ws_write(ti_ws_t * pss, ti_write_t * req)
{
    if (!pss->wsi)
        return 0;  /* ignore dead connections */

    if (queue_push(&pss->queue, req))
        return -1;

    lws_callback_on_writable(pss->wsi);
    return 0;
}

char * ti_ws_name(const char * prefix, ti_ws_t * pss)
{
    size_t n = strlen(prefix);
    struct lws_vhost * vhost = lws_get_vhost(pss->wsi);
    if (vhost)
    {
        const char * name = lws_get_vhost_iface(vhost);
        if (name)
        {
            size_t m = strlen(name);
            char * buffer = malloc(n + m + 1);

            memcpy(buffer, prefix, n);
            memcpy(buffer+n, name, m);
            buffer[n+m] = '\0';
            return buffer;
        }
    }
    return NULL;
}

void ti_ws_destroy(void)
{
    if (ws__context)
        lws_context_destroy(ws__context);
    ws__context = NULL;
}
