/*
 * ti/api.c
 */
#define _GNU_SOURCE
#include <ti/api.h>
#include <ti.h>
#include <stdlib.h>
#include <util/base64.h>
#include <util/logger.h>
#include <ti/auth.h>
#include <ti/access.h>
#include <ti/query.inline.h>

#define NOK_RESPONSE \
    "HTTP/1.1 503 Service Unavailable\r\n" \
    "Content-Type: text/plain\r\n" \
    "Content-Length: 4\r\n" \
    "\r\n" \
    "NOK\n"

#define UNAUTHORIZED_RESPONSE \
    "HTTP/1.1 401 Unauthorized\r\n" \
    "Content-Type: text/plain\r\n" \
    "Content-Length: 13\r\n" \
    "\r\n" \
    "UNAUTHORIZED\n"

#define NFOUND_RESPONSE \
    "HTTP/1.1 404 Not Found\r\n" \
    "Content-Type: text/plain\r\n" \
    "Content-Length: 10\r\n" \
    "\r\n" \
    "NOT FOUND\n"

#define UNSUPPORTED_RESPONSE \
    "HTTP/1.1 415 Unsupported Media Type\r\n" \
    "Content-Type: text/plain\r\n" \
    "Content-Length: 23\r\n" \
    "\r\n" \
    "UNSUPPORTED MEDIA TYPE\n"


#define CONTENT_TYPE_JSON "application/json"
#define CONTENT_TYPE_MSGPACK "application/msgpack"


#define API__ICMP_WITH(__s, __n, __w) \
    __n == strlen(__w) && strncasecmp(__s, __w, __n) == 0

/* static response buffers */
static uv_buf_t api__uv_nok_buf;
static uv_buf_t api__uv_unauthorized_buf;
static uv_buf_t api__uv_nfound_buf;
static uv_buf_t api__uv_unsupported_buf;

static uv_tcp_t api__uv_server;
static http_parser_settings api__settings;

static inline _Bool api__starts_with(
        const char ** str,
        size_t * strn,
        const char * with,
        size_t withn)
{
    if (*strn < withn || strncasecmp(*str, with, withn))
        return false;
    *str += withn;
    *strn -= withn;
    return true;
}

static void api__close_cb(uv_handle_t * handle)
{
    ti_api_request_t * api_request = handle->data;
    ti_user_drop(api_request->user);
    free(api_request->content);
    free(api_request);
}

static void api__alloc_cb(
        uv_handle_t * UNUSED(handle),
        size_t UNUSED(sugsz),
        uv_buf_t * buf)
{
    buf->base = malloc(HTTP_MAX_HEADER_SIZE);
    buf->len = buf->base ? HTTP_MAX_HEADER_SIZE-1 : 0;
}

static void api__data_cb(
        uv_stream_t * uvstream,
        ssize_t n,
        const uv_buf_t * buf)
{
    size_t parsed;
    ti_api_request_t * api_request = uvstream->data;

    if (api_request->flags & TI_API_FLAG_IS_CLOSED)
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

    api_request = calloc(1, sizeof(ti_api_request_t));
    if (!api_request)
    {
        log_critical(EX_MEMORY_S);
        return;
    }

    (void) uv_tcp_init(ti()->loop, (uv_tcp_t *) &api_request->uvstream);

    api_request->_id = TI_API_IDENTIFIER;
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

static int api__url_cb(http_parser * parser, const char * at, size_t n)
{
    ti_scope_t scope;
    ti_api_request_t * api_request = parser->data;

    if (ti_scope_init_uri(&scope, at, n))
    {
        log_debug("URI (scope) not found: %.*s", (int) n, at);
        api_request->flags |= TI_API_FLAG_INVALID_SCOPE;
    }

    return 0;
}

static int api__header_field_cb(http_parser * parser, const char * at, size_t n)
{
    ti_api_request_t * api_request = parser->data;

    if (API__ICMP_WITH(at, n, "content-type"))
    {
        api_request->state = TI_API_STATE_CONTENT_TYPE;
        return 0;
    }

    if (API__ICMP_WITH(at, n, "authorization"))
    {
        api_request->state = TI_API_STATE_AUTHORIZATION;
        return 0;
    }

    api_request->state = TI_API_STATE_NONE;
    return 0;
}

static int api__header_value_cb(http_parser * parser, const char * at, size_t n)
{
    ti_api_request_t * api_request = parser->data;

    switch (api_request->state)
    {
    case TI_API_STATE_NONE:
        break;

    case TI_API_STATE_CONTENT_TYPE:
        if (API__ICMP_WITH(at, n, CONTENT_TYPE_JSON))
        {
            api_request->content_type = TI_API_CT_JSON;
            break;
        }
        if ((API__ICMP_WITH(at, n, CONTENT_TYPE_MSGPACK)) ||
            (API__ICMP_WITH(at, n, "application/x-msgpack")))
        {
            api_request->content_type = TI_API_CT_MSGPACK;
            break;
        }

        /* invalid content type */
        log_debug("unsupported content-type: %.*s", (int) n, at);
        break;

    case TI_API_STATE_AUTHORIZATION:
        if (api__starts_with(&at, &n, "token ", strlen("token ")))
        {
            mp_obj_t mp_t;
            mp_t.tp = MP_STR;
            mp_t.via.str.n = n;
            mp_t.via.str.data = at;
            api_request->user = ti_users_auth_by_token(&mp_t, &api_request->e);

            if (api_request->user)
                ti_incref(api_request->user);

            break;
        }

        if (api__starts_with(&at, &n, "basic ", strlen("basic ")))
        {
            api_request->user = ti_users_auth_by_basic(at, n, &api_request->e);

            if (api_request->user)
                ti_incref(api_request->user);

            break;
        }

        log_debug("invalid authorization type: %.*s", (int) n, at);
        break;
    }
    return 0;
}

static int api__headers_complete_cb(http_parser * parser)
{
    ti_api_request_t * api_request = parser->data;

    assert (!api_request->content);

    api_request->content = malloc(parser->content_length);
    if (api_request->content)
        api_request->content_n = parser->content_length;

    return 0;
}

static int api__body_cb(http_parser * parser, const char * at, size_t n)
{
    size_t offset;
    ti_api_request_t * api_request = parser->data;

    if (!n || !api_request->content_n)
        return 0;

    offset = api_request->content_n - (parser->content_length + n);
    assert (offset + n <= api_request->content_n);
    memcpy(api_request->content + offset, at, n);

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

static void api__write_free_cb(uv_write_t * req, int status)
{
    free(req->data);
    api__write_cb(req, status);
}

void ti_api_err_response(ti_api_request_t * api_request)
{
    assert(api_request->e.nr);
    assert(api_request->content_type);
    free(api_request->content);
}


static int api__err(char ** ptr, const char * message, ex_t * e)
{
    return asprintf(
        ptr,
        "HTTP/1.1 %s\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %zu\r\n"
        "\r\n"
        "%s (%04d)\n",
        message,
        e->n + strlen(" (-000)\n"),
        e->msg,
        e->nr);
}

int ti_api_close_with_err(ti_api_request_t * api_request)
{
    assert (api_request->e.nr);

    char * response = NULL;
    int size = 0;
    ex_t * e = &api_request->e;

    switch (e->nr)
    {
    case EX_OPERATION_ERROR:
    case EX_NUM_ARGUMENTS:
    case EX_TYPE_ERROR:
    case EX_VALUE_ERROR:
    case EX_OVERFLOW:
    case EX_ZERO_DIV:
    case EX_MAX_QUOTA:
    case EX_LOOKUP_ERROR:
    case EX_BAD_DATA:
    case EX_SYNTAX_ERROR:
    case EX_ASSERT_ERROR:
        size = api__err(&response, "400 Bad Request", e);
        break;
    case EX_AUTH_ERROR:
        size = api__err(&response, "401 Unauthorized", e);
        break;
    case EX_FORBIDDEN:
        size = api__err(&response, "403 Forbidden", e);
        break;
    case EX_NODE_ERROR:
        size = api__err(&response, "503 Service Unavailable", e);
        break;
    case EX_RESULT_TOO_LARGE:
    case EX_REQUEST_TIMEOUT:
    case EX_REQUEST_CANCEL:
    case EX_WRITE_UV:
    case EX_MEMORY:
    case EX_INTERNAL:
        size = api__err(&response, "500 Internal Server Error", e);
        break;

    case EX_SUCCESS:
    case EX_RETURN:
        assert (0);
    }

    if (size > 0)
    {
        uv_buf_t uvbuf = uv_buf_init(response, (unsigned int) size);

        /* bind response to request to we can free in the callback */
        api_request->req.data = response;

        (void) uv_write(
                &api_request->req,
                &api_request->uvstream,
                &uvbuf,
                1,
                api__write_free_cb);
        return 0;
    }

    /* error, close */
    free(response);
    ti_api_close(api_request);
    return -1;
}

typedef struct
{
    mp_obj_t mp_type;
    mp_obj_t mp_query;
    mp_obj_t mp_procedure;
    mp_obj_t mp_args;
    mp_obj_t mp_vars;
} api__req_t;

static int api__run(ti_api_request_t * api_request, api__req_t * req)
{
    ex_t * e = &api_request->e;
    return e->nr;
}

static int api__query(ti_api_request_t * api_request, api__req_t * req)
{
    vec_t * access_;
    ti_query_t * query;
    ex_t * e = &api_request->e;

    if (req->mp_query.tp != MP_STR)
        goto invalid_api_request;

    if (req->mp_vars.tp && req->mp_vars.tp != MP_MAP)
        goto invalid_api_request;

query:
    query = ti_query_create(api_request, api_request->user, 0);

    if (!query || !(query->querystr = mp_strdup(&req->mp_query)))
    {
        ex_set_mem(e);
        goto finish;
    }

    if (ti_query_apply_scope(query, &api_request->scope, e))
        goto finish;

    if (req->mp_vars.tp)
    {
        mp_unp_t up;
        mp_unp_init(&up, req->mp_vars.via.bin.data, req->mp_vars.via.bin.n);
        ti_vup_t vup = {
                .isclient = true,
                .collection = query->collection,
                .up = &up,
        };

        if (ti_query_unpack_args(query, &vup, e))
            goto finish;
    }

    access_ = ti_query_access(query);
    assert (access_);

    if (ti_access_check_err(access_, query->user, TI_AUTH_READ, e) ||
        ti_query_parse(query, e) ||
        ti_query_investigate(query, e))
        goto finish;

    if (ti_query_will_update(query))
    {
        assert (api_request->scope.tp != TI_SCOPE_NODE);

        if (ti_access_check_err(access_, query->user, TI_AUTH_MODIFY, e) ||
            ti_events_create_new_event(query, e))
            goto finish;

        return 0;
    }

    ti_query_run(query);
    return 0;

invalid_api_request:
    ex_set(e, EX_BAD_DATA, "invalid API request"DOC_API_REQUEST);
finish:
    ti_query_destroy(query);
    return e->nr;
}

static void api__from_msgpack(ti_api_request_t * api_request)
{
    api__req_t request = {0};
    mp_unp_t up;
    mp_obj_t obj, mp_key;
    ex_t * e = &api_request->e;
    size_t i;
    mp_unp_init(&up, api_request->content, api_request->content_n);

    if (mp_next(&up, &obj) != MP_MAP || obj.via.sz < 2)
        goto invalid_api_request;

    for (i = obj.via.sz; i--;)
    {
        if (mp_next(&up, &mp_key) != MP_STR)
            goto invalid_api_request;

        if (mp_str_eq(&mp_key, "type"))
            mp_next(&up, &request.mp_type);

        else if (mp_str_eq(&mp_key, "query"))
            mp_next(&up, &request.mp_query);

        else if (mp_str_eq(&mp_key, "procedure"))
            mp_next(&up, &request.mp_procedure);

        else if (mp_str_eq(&mp_key, "vars"))
        {
            request.mp_vars.via.str.data = up.pt;
            if (mp_skip(&up) != MP_MAP)
                goto invalid_api_request;
            request.mp_vars.via.str.n = up.pt - request.mp_vars.via.str.data;
            request.mp_vars.tp = MP_BIN;
        }

        else if (mp_str_eq(&mp_key, "args"))
        {
            request.mp_args.via.str.data = up.pt;
            if (mp_skip(&up) != MP_ARR)
                goto invalid_api_request;
            request.mp_args.via.str.n = up.pt - request.mp_args.via.str.data;
            request.mp_args.tp = MP_BIN;
        }

        else
        {
            log_warning(
                    "unknown key in API request: `%.*s`",
                    (int) mp_key.via.str.n,
                    mp_key.via.str.data);
            mp_skip(&up);
        }
    }

    if (request.mp_type.tp == 0)
        goto invalid_api_request;

    if (mp_str_eq(&request.mp_type, "query"))
    {
        if (api__query(api_request, &request))
            goto failed;
        return;
    }

    if (mp_str_eq(&request.mp_type, "run"))
    {
        if (api__run(api_request, &request))
            goto failed;
        return;
    }

invalid_api_request:
    ex_set(e, EX_BAD_DATA, "invalid API request"DOC_API_REQUEST);
failed:
    ++ti()->counters->queries_with_error;
    ti_api_close_with_err(api_request);
}


static int api__message_complete_cb(http_parser * parser)
{
    ti_api_request_t * api_request = parser->data;

    if (api_request->flags & TI_API_FLAG_INVALID_SCOPE)
    {
        (void) uv_write(
                &api_request->req,
                &api_request->uvstream,
                &api__uv_nfound_buf, 1,
                api__write_cb);
        return 0;
    }

    if (!api_request->user)
    {
        (void) uv_write(
                &api_request->req,
                &api_request->uvstream,
                &api__uv_unauthorized_buf, 1,
                api__write_cb);
        return 0;
    }

    switch (api_request->content_type)
    {
    case TI_API_CT_UNKNOWN:
        (void) uv_write(
                &api_request->req,
                &api_request->uvstream,
                &api__uv_unsupported_buf, 1,
                api__write_cb);
        return 0;
    case TI_API_CT_JSON:
        assert (0);
        break;
    case TI_API_CT_MSGPACK:
        assert (api_request->e.nr == 0);
        api__from_msgpack(api_request);
        return 0;

    }

    (void) uv_write(
            &api_request->req,
            &api_request->uvstream,
            &api__uv_nok_buf, 1,
            api__write_cb);

    return 0;
}

static int api__chunk_header_cb(http_parser * parser)
{
    LOGC("Chunk header\n Content-Length: %zu", parser->content_length);
    return 0;
}

static int api__chunk_complete_cb(http_parser * parser)
{
    LOGC("Chunk complete\n Content-Length: %zu", parser->content_length);
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
    api__uv_unauthorized_buf =
            uv_buf_init(UNAUTHORIZED_RESPONSE, strlen(UNAUTHORIZED_RESPONSE));
    api__uv_nfound_buf =
            uv_buf_init(NFOUND_RESPONSE, strlen(NFOUND_RESPONSE));
    api__uv_unsupported_buf =
            uv_buf_init(UNSUPPORTED_RESPONSE, strlen(UNSUPPORTED_RESPONSE));

    api__settings.on_url = api__url_cb;
    api__settings.on_header_field = api__header_field_cb;
    api__settings.on_header_value = api__header_value_cb;
    api__settings.on_message_complete = api__message_complete_cb;
    api__settings.on_body = api__body_cb;
    api__settings.on_chunk_header = api__chunk_header_cb;
    api__settings.on_chunk_complete = api__chunk_complete_cb;
    api__settings.on_headers_complete = api__headers_complete_cb;

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

ti_api_request_t * ti_api_acquire(ti_api_request_t * api_request)
{
    api_request->flags |= TI_API_FLAG_IN_USE;
    return api_request;
}

void ti_api_release(ti_api_request_t * api_request)
{
    api_request->flags &= ~TI_API_FLAG_IN_USE;

    if (api_request->flags & TI_API_FLAG_IS_CLOSED)
        uv_close((uv_handle_t *) &api_request->uvstream, api__close_cb);
}

void ti_api_close(ti_api_request_t * api_request)
{
    if (!api_request || (api_request->flags & TI_API_FLAG_IS_CLOSED))
        return;

    api_request->flags |= TI_API_FLAG_IS_CLOSED;

    if (api_request->flags & TI_API_FLAG_IN_USE)
        return;

    uv_close((uv_handle_t *) &api_request->uvstream, api__close_cb);
}


