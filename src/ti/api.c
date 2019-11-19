/*
 * ti/api.c
 */
#include <ti/api.h>
#include <ti.h>
#include <util/logger.h>

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


/* static response buffers */
static uv_buf_t api__uv_nok_buf;
static uv_buf_t api__uv_nfound_buf;

static uv_tcp_t api__uv_server;
static http_parser_settings api__settings;

static void api__close_cb(uv_handle_t * handle)
{
    ti_api_request_t * api_request = handle->data;
    free(api_request);
}

static void api__alloc_cb(
        uv_handle_t * UNUSED(handle),
        size_t UNUSED(sugsz),
        uv_buf_t * buf)
{
    buf->base = malloc(HTTP_MAX_HEADER_SIZE);
    buf->len = buf->base ? HTTP_MAX_HEADER_SIZE : 0;
}

static void api__data_cb(
        uv_stream_t * uvstream,
        ssize_t n,
        const uv_buf_t * buf)
{
    size_t parsed;
    ti_api_request_t * api_request = uvstream->data;

    if (api_request->is_closed)
        goto done;

    if (n < 0)
    {
        if (n != UV_EOF)
            log_error(uv_strerror(n));
        ti_api_close(api_request);
        goto done;
    }

    buf->base[HTTP_MAX_HEADER_SIZE-1] = '\0';

    parsed = http_parser_execute(
            &api_request->parser,
            &api__settings,
            buf->base, n);

    if (api_request->parser.upgrade)
    {
        /* TODO: do we need to do something? */
        log_debug("upgrade to a new protocol");
    }
    else if (parsed != (size_t) n)
    {
        log_warning("error parsing HTTP API request");
        ti_api_close(api_request);
    }

done:
     free(buf->base);
}

static void api__connection_cb(uv_stream_t * server, int status)
{
    int rc;
    ti_api_request_t * api_request;

    if (status < 0)
    {
        log_error("HTTP API connection error: `%s`", uv_strerror(status));
        return;
    }

    log_debug("received a HTTP API call");

    api_request = malloc(sizeof(ti_api_request_t));
    if (!api_request)
    {
        log_critical(EX_MEMORY_S);
        return;
    }

    (void) uv_tcp_init(ti()->loop, (uv_tcp_t *) &api_request->uvstream);

    api_request->_id = TI_API_IDENTIFIER;
    api_request->is_closed = false;
    api_request->uvstream.data = api_request;
    api_request->parser.data = api_request;

    rc = uv_accept(server, &api_request->uvstream);
    if (rc)
    {
        log_error("cannot accept HTTP API request: `%s`", uv_strerror(rc));
        ti_api_close(api_request);
        return;
    }

    http_parser_init(&api_request->parser, HTTP_REQUEST);

    rc = uv_read_start(&api_request->uvstream, api__alloc_cb, api__data_cb);
    if (rc)
    {
        log_error("cannot read HTTP API request: `%s`", uv_strerror(rc));
        ti_api_close(api_request);
        return;
    }
}

static int api__url_cb(http_parser * parser, const char * at, size_t length)
{
    ti_scope_t scope;
    ti_api_request_t * api_request = parser->data;

    if (ti_scope_init_uri(&scope, at, length) == 0)
    {
        LOGC("scope: %d", scope.tp);
    }

    api_request->response = &api__uv_nfound_buf;

    return 0;
}

static void api__write_cb(uv_write_t * req, int status)
{
    if (status)
        log_error(
                "error writing HTTP API response: `%s`",
                uv_strerror(status));

    ti_api_close((ti_api_request_t *) req->handle->data);
}

static int api__message_complete_cb(http_parser * parser)
{
    ti_api_request_t * api_request = parser->data;

    if (!api_request->response)
        api_request->response = &api__uv_nfound_buf;

    (void) uv_write(
            &api_request->req,
            &api_request->uvstream,
            api_request->response, 1,
            api__write_cb);

    return 0;
}


int ti_api_init(void)
{
    int rc;
    struct sockaddr_storage addr = {0};
    uint16_t port = ti()->cfg->http_api_port;

    if (port == 0)
        return 0;

    (void) uv_ip6_addr("::", (int) port, (struct sockaddr_in6 *) &addr);

    api__uv_nok_buf =
            uv_buf_init(NOK_RESPONSE, strlen(NOK_RESPONSE));
    api__uv_nfound_buf =
            uv_buf_init(NFOUND_RESPONSE, strlen(NFOUND_RESPONSE));

    api__settings.on_url = api__url_cb;
    api__settings.on_message_complete = api__message_complete_cb;

    if (
        (rc = uv_tcp_init(ti()->loop, &api__uv_server)) ||
        (rc = uv_tcp_bind(
                &api__uv_server,
                (const struct sockaddr *) &addr,
                0)) ||
        (rc = uv_listen(
                (uv_stream_t *) &api__uv_server,
                128,
                api__connection_cb)))
    {
        log_error("error initializing HTTP API on port %u: `%s`",
                port,
                uv_strerror(rc));
        return -1;
    }

    log_info("start listening for HTTP API requests on TCP port %u", port);
    return 0;
}

void ti_api_close(ti_api_request_t * api_request)
{
    if (!api_request || api_request->is_closed)
        return;
    api_request->is_closed = true;
    uv_close((uv_handle_t *) &api_request->uvstream, api__close_cb);
}
