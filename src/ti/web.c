/*
 * ti/web.c
 */
#include <ti/web.h>
#include <ti.h>
#include <util/logger.h>

#define OK_RESPONSE \
    "HTTP/1.1 200 OK\r\n" \
    "Content-Type: text/plain\r\n" \
    "Content-Length: 3\r\n" \
    "\r\n" \
    "OK\n"

#define NOK_RESPONSE \
    "HTTP/1.1 503 Service Unavailable\r\n" \
    "Content-Type: text/plain\r\n" \
    "Content-Length: 4\r\n" \
    "\r\n" \
    "NOK\n"

#define NFOUND_RESPONSE \
    "HTTP/1.1 404 Not Found\r\n" \
    "Content-Type: text/plain\r\n" \
    "Content-Length: 10\r\n" \
    "\r\n" \
    "NOT FOUND\n"

#define MNA_RESPONSE \
    "HTTP/1.1 405 Method Not Allowed\r\n" \
    "Content-Type: text/plain\r\n" \
    "Content-Length: 19\r\n" \
    "\r\n" \
    "METHOD NOT ALLOWED\n"

#define SYNC_RESPONSE \
    "HTTP/1.1 200 OK\r\n" \
    "Content-Type: text/plain\r\n" \
    "Content-Length: 14\r\n" \
    "\r\n" \
    "SYNCHRONIZING\n"

#define AWAY_RESPONSE \
    "HTTP/1.1 200 OK\r\n" \
    "Content-Type: text/plain\r\n" \
    "Content-Length: 5\r\n" \
    "\r\n" \
    "AWAY\n"

#define READY_RESPONSE \
    "HTTP/1.1 200 OK\r\n" \
    "Content-Type: text/plain\r\n" \
    "Content-Length: 6\r\n" \
    "\r\n" \
    "READY\n"

/* static response buffers */
static uv_buf_t web__uv_ok_buf;
static uv_buf_t web__uv_nok_buf;
static uv_buf_t web__uv_nfound_buf;
static uv_buf_t web__uv_mna_buf;
static uv_buf_t web__uv_sync_buf;
static uv_buf_t web__uv_away_buf;
static uv_buf_t web__uv_ready_buf;

static uv_tcp_t web__uv_server;
static http_parser_settings web__settings;

static void web__close_cb(uv_handle_t * handle)
{
    ti_web_request_t * web_request = handle->data;
    free(web_request);
}

static void web__alloc_cb(
        uv_handle_t * UNUSED(handle),
        size_t UNUSED(sugsz),
        uv_buf_t * buf)
{
    buf->base = malloc(HTTP_MAX_HEADER_SIZE);
    buf->len = buf->base ? HTTP_MAX_HEADER_SIZE-1 : 0;
}

static void web__data_cb(
        uv_stream_t * uvstream,
        ssize_t n,
        const uv_buf_t * buf)
{
    size_t parsed;
    ti_web_request_t * web_request = uvstream->data;

    if (web_request->is_closed)
        goto done;

    if (n < 0)
    {
        if (n != UV_EOF)
            log_error(uv_strerror(n));
        ti_web_close(web_request);
        goto done;
    }

    buf->base[HTTP_MAX_HEADER_SIZE-1] = '\0';

    parsed = http_parser_execute(
            &web_request->parser,
            &web__settings,
            buf->base, n);

    if (web_request->parser.upgrade)
    {
        /* TODO: do we need to do something? */
        log_debug("upgrade to a new protocol");
    }
    else if (parsed != (size_t) n)
    {
        char addr[INET6_ADDRSTRLEN];
        if (ti_tcp_addr(addr, (uv_tcp_t *) uvstream) == 0)
        {
            log_warning("error parsing HTTP status request from %s", addr);
        }
        else
        {
            /* cannot read address */
            log_warning("error parsing HTTP status request");
        }
        ti_web_close(web_request);
    }

done:
     free(buf->base);
}

static uv_buf_t * web__get_status_response(void)
{
    ti_node_t * this_node = ti()->node;

    if (this_node) switch ((ti_node_status_t) this_node->status)
    {
    case TI_NODE_STAT_OFFLINE:
    case TI_NODE_STAT_CONNECTING:
    case TI_NODE_STAT_BUILDING:
    case TI_NODE_STAT_SHUTTING_DOWN:
        break;
    case TI_NODE_STAT_SYNCHRONIZING:
        return &web__uv_sync_buf;
    case TI_NODE_STAT_AWAY:
    case TI_NODE_STAT_AWAY_SOON:
        return  &web__uv_away_buf;
    case TI_NODE_STAT_READY:
        return &web__uv_ready_buf;
    }
    return &web__uv_nok_buf;
}

static uv_buf_t * web__get_ready_response(void)
{
    ti_node_t * this_node = ti()->node;

    return (this_node && (
            /*
             * Exclude SYNCHRONIZATION although clients could connect
             * during this status, this is because the `ready` handler
             * can be used to indicate a shutdown of another node during
             * an upgrade cycle.
             */
            this_node->status & (
                TI_NODE_STAT_AWAY|
                TI_NODE_STAT_AWAY_SOON|
                TI_NODE_STAT_READY)
        )) ? &web__uv_ok_buf : &web__uv_nok_buf;
}

static int web__url_cb(http_parser * parser, const char * at, size_t length)
{
    ti_web_request_t * web_request = parser->data;

    web_request->response

        /* status response */
        = ((length == 1 && *at == '/') ||
           (length == 7 && memcmp(at, "/status", 7) == 0))
        ? web__get_status_response()

        /* ready response */
        : (length == 6 && memcmp(at, "/ready", 6) == 0)
        ? web__get_ready_response()

        /* healthy response */
        : (length == 8 && memcmp(at, "/healthy", 8) == 0)
        ? &web__uv_ok_buf

        /* everything else */
        : &web__uv_nfound_buf;

    return 0;
}

static void web__write_cb(uv_write_t * req, int status)
{
    if (status)
        log_error(
                "error writing HTTP status response: `%s`",
                uv_strerror(status));

    ti_web_close((ti_web_request_t *) req->handle->data);
}

static int web__message_complete_cb(http_parser * parser)
{
    ti_web_request_t * web_request = parser->data;

    (void) uv_write(
            &web_request->req,
            &web_request->uvstream,
            parser->method == HTTP_GET
                ? web_request->response
                : &web__uv_mna_buf,
            1,
            web__write_cb);

    return 0;
}

static void web__connection_cb(uv_stream_t * server, int status)
{
    int rc;
    ti_web_request_t * web_request;

    if (status < 0)
    {
        log_error("HTTP status connection error: `%s`", uv_strerror(status));
        return;
    }

    log_debug("received a HTTP status connection request");

    web_request = malloc(sizeof(ti_web_request_t));
    if (!web_request)
    {
        log_critical(EX_MEMORY_S);
        return;
    }

    (void) uv_tcp_init(ti()->loop, (uv_tcp_t *) &web_request->uvstream);

    web_request->_id = TI_WEB_IDENTIFIER;
    web_request->is_closed = false;
    web_request->uvstream.data = web_request;
    web_request->parser.data = web_request;
    web_request->response = &web__uv_nfound_buf;

    rc = uv_accept(server, &web_request->uvstream);
    if (rc)
    {
        log_error("cannot accept HTTP status request: `%s`", uv_strerror(rc));
        ti_web_close(web_request);
        return;
    }

    http_parser_init(&web_request->parser, HTTP_REQUEST);

    rc = uv_read_start(&web_request->uvstream, web__alloc_cb, web__data_cb);
    if (rc)
    {
        log_error("cannot read HTTP status request: `%s`", uv_strerror(rc));
        ti_web_close(web_request);
        return;
    }
}

int ti_web_init(void)
{
    int rc;
    struct sockaddr_storage addr = {0};
    uint16_t port = ti()->cfg->http_status_port;

    (void) uv_ip6_addr("::", (int) port, (struct sockaddr_in6 *) &addr);

    web__uv_ok_buf =
            uv_buf_init(OK_RESPONSE, strlen(OK_RESPONSE));
    web__uv_nok_buf =
            uv_buf_init(NOK_RESPONSE, strlen(NOK_RESPONSE));
    web__uv_nfound_buf =
            uv_buf_init(NFOUND_RESPONSE, strlen(NFOUND_RESPONSE));
    web__uv_mna_buf =
            uv_buf_init(MNA_RESPONSE, strlen(MNA_RESPONSE));
    web__uv_sync_buf =
            uv_buf_init(SYNC_RESPONSE, strlen(SYNC_RESPONSE));
    web__uv_away_buf =
            uv_buf_init(AWAY_RESPONSE, strlen(AWAY_RESPONSE));
    web__uv_ready_buf =
            uv_buf_init(READY_RESPONSE, strlen(READY_RESPONSE));

    web__settings.on_url = web__url_cb;
    web__settings.on_message_complete = web__message_complete_cb;

    if (
        (rc = uv_tcp_init(ti()->loop, &web__uv_server)) ||
        (rc = uv_tcp_bind(
                &web__uv_server,
                (const struct sockaddr *) &addr,
                0)) ||
        (rc = uv_listen(
                (uv_stream_t *) &web__uv_server,
                128,
                web__connection_cb)))
    {
        log_error("error initializing HTTP status server on port %u: `%s`",
                port,
                uv_strerror(rc));
        return -1;
    }

    log_info("start listening for HTTP status requests on TCP port %u", port);
    return 0;
}

void ti_web_close(ti_web_request_t * web_request)
{
    if (!web_request || web_request->is_closed)
        return;
    web_request->is_closed = true;
    uv_close((uv_handle_t *) &web_request->uvstream, web__close_cb);
}
