/*
 * ti/api.c
 */
#define _GNU_SOURCE
#include <ti/api.h>
#include <ti.h>
#include <stdlib.h>
#include <util/mpjson.h>
#include <util/logger.h>
#include <ti/auth.h>
#include <ti/access.h>
#include <ti/query.inline.h>


#define API__HEADER_MAX_SZ 256

#define CONTENT_TYPE_JSON "application/json"
#define CONTENT_TYPE_MSGPACK "application/msgpack"

static const char api__content_type[3][20] = {
        "text/plain",
        "application/json",
        "application/msgpack",
};

static const char api__html_header[9][32] = {
        "200 OK",
        "400 Bad Request",
        "401 Unauthorized",
        "403 Forbidden",
        "404 Not Found",
        "405 Method Not Allowed",
        "415 Unsupported Media Type",
        "500 Internal Server Error",
        "503 Service Unavailable",
};

static const char api__default_body[9][30] = {
        "OK\r\n",
        "BAD REQUEST\r\n",
        "UNAUTHORIZED\r\n",
        "FORBIDDEN\r\n",
        "NOT FOUND\r\n",
        "METHOD NOT ALLOWED\r\n",
        "UNSUPPORTED MEDIA TYPE\r\n",
        "INTERNAL SERVER ERROR\r\n",
        "SERVICE UNAVAILABLE\r\n",
};

typedef enum
{
    E200_OK,
    E400_BAD_REQUEST,
    E401_UNAUTHORIZED,
    E403_FORBIDDEN,
    E404_NOT_FOUND,
    E405_METHOD_NOT_ALLOWED,
    E415_UNSUPPORTED_MEDIA_TYPE,
    E500_INTERNAL_SERVER_ERROR,
    E503_SERVICE_UNAVAILABLE
} api__header_t;

#define API__ICMP_WITH(__s, __n, __w) \
    __n == strlen(__w) && strncasecmp(__s, __w, __n) == 0

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
    ti_api_request_t * ar = handle->data;
    ti_user_drop(ar->user);
    free(ar->content);
    free(ar);
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
    ti_api_request_t * ar = uvstream->data;

    if (ar->flags & TI_API_FLAG_IS_CLOSED)
        goto done;

    if (n < 0)
    {
        if (n != UV_EOF)
            log_error(uv_strerror(n));

        ti_api_close(ar);
        goto done;
    }

    buf->base[HTTP_MAX_HEADER_SIZE-1] = '\0';

    parsed = http_parser_execute(
            &ar->parser,
            &api__settings,
            buf->base, n);

    if (ar->parser.upgrade)
    {
        /* TODO: do we need to do something? */
        log_debug("upgrade to a new protocol");
    }
    else if (parsed != (size_t) n)
    {
        log_warning("error parsing HTTP API request");
        ti_api_close(ar);
    }

done:
     free(buf->base);
}

static void api__connection_cb(uv_stream_t * server, int status)
{
    int rc;
    ti_api_request_t * ar;

    if (status < 0)
    {
        log_error("HTTP API connection error: `%s`", uv_strerror(status));
        return;
    }

    log_debug("received a HTTP API call");

    ar = calloc(1, sizeof(ti_api_request_t));
    if (!ar)
    {
        log_critical(EX_MEMORY_S);
        return;
    }

    (void) uv_tcp_init(ti()->loop, (uv_tcp_t *) &ar->uvstream);

    ar->_id = TI_API_IDENTIFIER;
    ar->uvstream.data = ar;
    ar->parser.data = ar;

    rc = uv_accept(server, &ar->uvstream);
    if (rc)
    {
        log_error("cannot accept HTTP API request: `%s`", uv_strerror(rc));
        ti_api_close(ar);
        return;
    }

    http_parser_init(&ar->parser, HTTP_REQUEST);

    rc = uv_read_start(&ar->uvstream, api__alloc_cb, api__data_cb);
    if (rc)
    {
        log_error("cannot read HTTP API request: `%s`", uv_strerror(rc));
        ti_api_close(ar);
        return;
    }
}

static int api__url_cb(http_parser * parser, const char * at, size_t n)
{
    ti_api_request_t * ar = parser->data;

    if (ti_scope_init(&ar->scope, at, n, &ar->e))
    {
        log_debug("URI (scope) not found: %s", ar->e.msg);
        ar->flags |= TI_API_FLAG_INVALID_SCOPE;
    }

    return 0;
}

static int api__header_field_cb(http_parser * parser, const char * at, size_t n)
{
    ti_api_request_t * ar = parser->data;

    ar->state = API__ICMP_WITH(at, n, "content-type")
            ? TI_API_STATE_CONTENT_TYPE
            : API__ICMP_WITH(at, n, "authorization")
            ? TI_API_STATE_AUTHORIZATION
            : TI_API_STATE_NONE;

    return 0;
}

static int api__header_value_cb(http_parser * parser, const char * at, size_t n)
{
    ti_api_request_t * ar = parser->data;

    switch (ar->state)
    {
    case TI_API_STATE_NONE:
        break;

    case TI_API_STATE_CONTENT_TYPE:
        if (API__ICMP_WITH(at, n, CONTENT_TYPE_MSGPACK))
        {
            ar->content_type = TI_API_CT_MSGPACK;
            break;
        }
        if (API__ICMP_WITH(at, n, CONTENT_TYPE_JSON))
        {
            ar->content_type = TI_API_CT_JSON;
            break;
        }
        if (API__ICMP_WITH(at, n, "application/x-msgpack"))
        {
            ar->content_type = TI_API_CT_MSGPACK;
            break;
        }
        if (API__ICMP_WITH(at, n, "text/json"))
        {
            ar->content_type = TI_API_CT_JSON;
            break;
        }

        /* invalid content type */
        log_debug("unsupported content-type: %.*s", (int) n, at);
        break;

    case TI_API_STATE_AUTHORIZATION:
        if (api__starts_with(&at, &n, "Token ", strlen("Token ")) ||
            api__starts_with(&at, &n, "Bearer ", strlen("Bearer ")))
        {
            mp_obj_t mp_t;
            mp_t.tp = MP_STR;
            mp_t.via.str.n = n;
            mp_t.via.str.data = at;
            ar->user = ti_users_auth_by_token(&mp_t, &ar->e);

            if (ar->user)
                ti_incref(ar->user);

            break;
        }

        if (api__starts_with(&at, &n, "Basic ", strlen("Basic ")))
        {
            ar->user = ti_users_auth_by_basic(at, n, &ar->e);

            if (ar->user)
                ti_incref(ar->user);

            break;
        }

        log_debug("invalid authorization type: %.*s", (int) n, at);
        break;
    }
    return 0;
}

static int api__headers_complete_cb(http_parser * parser)
{
    ti_api_request_t * ar = parser->data;

    assert (!ar->content);

    ar->content = malloc(parser->content_length);
    if (ar->content)
        ar->content_n = parser->content_length;

    return 0;
}

static int api__body_cb(http_parser * parser, const char * at, size_t n)
{
    size_t offset;
    ti_api_request_t * ar = parser->data;

    if (!n || !ar->content_n)
        return 0;

    offset = ar->content_n - (parser->content_length + n);
    assert (offset + n <= ar->content_n);
    memcpy(ar->content + offset, at, n);

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

inline static int api__header(
        char * ptr,
        const api__header_t ht,
        const ti_api_content_t ct,
        size_t content_length)
{
    int len = sprintf(
        ptr,
        "HTTP/1.1 %s\r\n" \
        "Content-Type: %s\r\n" \
        "Content-Length: %zu\r\n" \
        "\r\n",
        api__html_header[ht],
        api__content_type[ct],
        content_length);
    return len;
}

static inline int api__err_body(char ** ptr, ex_t * e)
{
    return asprintf(ptr, "%s (%d)\r\n", e->msg, e->nr);
}

static void api__set_yajl_gen_status_error(ex_t * e, yajl_gen_status stat)
{
    switch (stat)
    {
    case yajl_gen_status_ok:
        return;
    case yajl_gen_keys_must_be_strings:
        ex_set(e, EX_TYPE_ERROR, "JSON keys must be strings");
        return;
    case yajl_max_depth_exceeded:
        ex_set(e, EX_OPERATION_ERROR, "JSON max depth exceeded");
        return;
    case yajl_gen_in_error_state:
        ex_set(e, EX_INTERNAL, "JSON general error");
        return;
    case yajl_gen_generation_complete:
        ex_set(e, EX_INTERNAL, "JSON completed");
        return;
    case yajl_gen_invalid_number:
        ex_set(e, EX_TYPE_ERROR, "JSON invalid number");
        return;
    case yajl_gen_no_buf:
        ex_set(e, EX_INTERNAL, "JSON no buffer has been set");
        return;
    case yajl_gen_invalid_string:
        ex_set(e, EX_TYPE_ERROR, "JSON only accepts valid UTF8 strings");
        return;
    }
    ex_set(e, EX_INTERNAL, "JSON unexpected error");
}

int ti_api_close_with_json(ti_api_request_t * ar, void * data, size_t size)
{
    char header[API__HEADER_MAX_SZ];
    int header_size = 0;

    header_size = api__header(header, E200_OK, TI_API_CT_MSGPACK, size);

    uv_buf_t uvbufs[2] = {
            uv_buf_init(header, (unsigned int) header_size),
            uv_buf_init(data, size),
    };

    /* bind response to request to we can free in the callback */
    ar->req.data = data;

    uv_write(&ar->req, &ar->uvstream, uvbufs, 2, api__write_free_cb);
    return 0;
}

static int api__close_resp(ti_api_request_t * ar, void * data, size_t size)
{
    char header[API__HEADER_MAX_SZ];
    int header_size = 0;

    header_size = api__header(header, E200_OK, ar->content_type, size);

    uv_buf_t uvbufs[2] = {
            uv_buf_init(header, (unsigned int) header_size),
            uv_buf_init(data, size),
    };

    /* bind response to request to we can free in the callback */
    ar->req.data = data;

    uv_write(&ar->req, &ar->uvstream, uvbufs, 2, api__write_free_cb);
    return 0;
}

int ti_api_close_with_response(ti_api_request_t * ar, void * data, size_t size)
{
    if (ar->content_type == TI_API_CT_JSON)
    {
        size_t tmp_sz;
        unsigned char * tmp;
        yajl_gen_status stat = mpjson_mp_to_json(
                data,
                size,
                &tmp,
                &tmp_sz,
                ar->flags);
        free(data);
        if (stat)
        {
            api__set_yajl_gen_status_error(&ar->e, stat);
            return ti_api_close_with_err(ar, &ar->e);
        }
        data = tmp;
        size = tmp_sz;
    }
    return api__close_resp(ar, data, size);
}

int ti_api_close_with_err(ti_api_request_t * ar, ex_t * e)
{
    assert (e->nr);

    char header[API__HEADER_MAX_SZ];
    char * body = NULL;
    int header_size = 0, body_size = 0;

    switch (e->nr)
    {
    case EX_OPERATION_ERROR:
    case EX_NUM_ARGUMENTS:
    case EX_TYPE_ERROR:
    case EX_VALUE_ERROR:
    case EX_OVERFLOW:
    case EX_ZERO_DIV:
    case EX_MAX_QUOTA:
    case EX_BAD_DATA:
    case EX_SYNTAX_ERROR:
    case EX_ASSERT_ERROR:
        body_size = api__err_body(&body, e);
        header_size = api__header(
                header,
                E400_BAD_REQUEST,
                TI_API_CT_TEXT,
                (size_t) body_size);
        break;
    case EX_AUTH_ERROR:
        body_size = api__err_body(&body, e);
        header_size = api__header(
                header,
                E401_UNAUTHORIZED,
                TI_API_CT_TEXT,
                (size_t) body_size);
        break;
    case EX_FORBIDDEN:
        body_size = api__err_body(&body, e);
        header_size = api__header(
                header,
                E403_FORBIDDEN,
                TI_API_CT_TEXT,
                (size_t) body_size);
        break;
    case EX_LOOKUP_ERROR:
        body_size = api__err_body(&body, e);
        header_size = api__header(
                header,
                E404_NOT_FOUND,
                TI_API_CT_TEXT,
                (size_t) body_size);
        break;
    case EX_NODE_ERROR:
        body_size = api__err_body(&body, e);
        header_size = api__header(
                header,
                E503_SERVICE_UNAVAILABLE,
                TI_API_CT_TEXT,
                (size_t) body_size);
        break;
    case EX_RESULT_TOO_LARGE:
    case EX_REQUEST_TIMEOUT:
    case EX_REQUEST_CANCEL:
    case EX_WRITE_UV:
    case EX_MEMORY:
    case EX_INTERNAL:
        body_size = api__err_body(&body, e);
        header_size = api__header(
                header,
                E500_INTERNAL_SERVER_ERROR,
                TI_API_CT_TEXT,
                (size_t) body_size);
        break;

    case EX_SUCCESS:
    case EX_RETURN:
        assert (0);
    }

    if (header_size > 0 && body_size > 0)
    {
        uv_buf_t uvbufs[2] = {
                uv_buf_init(header, (unsigned int) header_size),
                uv_buf_init(body, (unsigned int) body_size),
        };

        /* bind response to request to we can free in the callback */
        ar->req.data = body;

        (void) uv_write(
                &ar->req,
                &ar->uvstream,
                uvbufs,
                2,
                api__write_free_cb);
        return 0;
    }

    /* error, close */
    free(body);
    ti_api_close(ar);
    return -1;
}

typedef struct
{
    mp_obj_t mp_type;
    mp_obj_t mp_code;
    mp_obj_t mp_procedure;
    mp_obj_t mp_args;
    mp_obj_t mp_vars;
} api__req_t;

static int api__gen_scope(ti_api_request_t * ar, msgpack_packer * pk)
{
    switch (ar->scope.tp)
    {
    case TI_SCOPE_THINGSDB:
        return mp_pack_str(pk, "@t");
    case TI_SCOPE_NODE:
        return mp_pack_fmt(pk, "@n:%d", ar->scope.via.node_id);
    case TI_SCOPE_COLLECTION_NAME:
        return mp_pack_fmt(pk, "@:%.*s",
                (int) ar->scope.via.collection_name.sz,
                ar->scope.via.collection_name.name);
    case TI_SCOPE_COLLECTION_ID:
        return mp_pack_fmt(pk, "@:%"PRIu64, ar->scope.via.collection_id);
    }
    return -1;
}

typedef int (*api__gen_data_cb)(
        ti_api_request_t *,
        api__req_t *,
        unsigned char **,
        size_t *);

static int api__gen_run_data(
        ti_api_request_t * ar,
        api__req_t * req,
        unsigned char ** data,
        size_t * size)
{
    size_t i = 0;
    mp_unp_t up = {0};
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, 0, 0);
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (req->mp_args.tp == MP_BIN)
    {
        mp_obj_t obj;
        mp_unp_init(&up, req->mp_args.via.bin.data, req->mp_args.via.bin.n);
        mp_next(&up, &obj);
        i = obj.via.sz;
    }

    if (msgpack_pack_array(&pk, 2 + i) ||
        api__gen_scope(ar, &pk) ||
        mp_pack_strn(
                &pk,
                req->mp_procedure.via.str.data,
                req->mp_procedure.via.str.n))
        goto fail;

    if (i && mp_pack_append(&pk, up.pt, up.end- up.pt))
        goto fail;

    *data = (unsigned char *) buffer.data;
    *size = buffer.size;

    return 0;

fail:
    msgpack_sbuffer_destroy(&buffer);
    return -1;
}

static int api__gen_query_data(
        ti_api_request_t * ar,
        api__req_t * req,
        unsigned char ** data,
        size_t * size)
{
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, 0, 0);
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (msgpack_pack_array(&pk, 2 + !!req->mp_vars.tp) ||
        api__gen_scope(ar, &pk) ||
        mp_pack_strn(&pk, req->mp_code.via.str.data, req->mp_code.via.str.n))
        goto fail;

    if (req->mp_vars.tp == MP_BIN &&
        mp_pack_append(&pk, req->mp_vars.via.bin.data, req->mp_vars.via.bin.n))
        goto fail;

    *data = (unsigned char *) buffer.data;
    *size = buffer.size;

    return 0;

fail:
    msgpack_sbuffer_destroy(&buffer);
    return -1;
}

static ti_pkg_t * api__gen_fwd_pkg(
        ti_api_request_t * ar,
        api__req_t * req,
        api__gen_data_cb gen_data_cb,
        ti_proto_enum_t proto)
{
    ti_pkg_t * pkg;
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    unsigned char * data;
    size_t size;

    if (mp_sbuffer_alloc_init(&buffer, 128 + ar->content_n, sizeof(ti_pkg_t)))
        return NULL;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    /* some extra size for setting the raw size + user_id */
    msgpack_pack_array(&pk, 2);
    msgpack_pack_uint64(&pk, ar->user->id);

    if (gen_data_cb(ar, req, &data, &size) ||
        mp_pack_bin(&pk, data, size))
    {
        msgpack_sbuffer_destroy(&buffer);
        return NULL;
    }

    free(data);

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, proto, buffer.size);

    return pkg;
}

static void api__fwd_cb(ti_req_t * req, ex_enum status)
{
    ti_api_request_t * ar = req->data;
    _Bool is_closed = ti_api_is_closed(ar);

    ti_api_release(ar);

    if (is_closed)
        goto done;

    if (status)
    {
        ex_set(&ar->e, status, ex_str(status));
        goto fail;
    }

    switch (req->pkg_res->tp)
    {
    case TI_PROTO_CLIENT_RES_ERROR:
        ti_pkg_client_err_to_e(&ar->e, req->pkg_res);
        goto fail;

    case TI_PROTO_CLIENT_RES_DATA:
        switch (ar->content_type)
        {
        case TI_API_CT_TEXT:
            break;
        case TI_API_CT_JSON:
        {
            size_t size;
            unsigned char * data;
            yajl_gen_status stat = mpjson_mp_to_json(
                    req->pkg_res->data,
                    req->pkg_res->n,
                    &data,
                    &size,
                    ar->flags);
            if (stat)
            {
                api__set_yajl_gen_status_error(&ar->e, stat);
                goto fail;
            }
            api__close_resp(ar, data, size);
            goto done;
        }
        case TI_API_CT_MSGPACK:
        {
            size_t size = req->pkg_res->n;
            char * data = malloc(size);
            if (!data)
            {
                ex_set_mem(&ar->e);
                goto fail;
            }
            memcpy(data, req->pkg_res->data, size);
            api__close_resp(ar, data, size);
            goto done;
        }
        }
    }

    ex_set_internal(&ar->e);

fail:
    ti_api_close_with_err(ar, &ar->e);
done:
    ti_req_destroy(req);
}

static int api__fwd(ti_api_request_t * ar, ti_node_t * to_node, ti_pkg_t * pkg)
{
    ti_api_acquire(ar);

    if (ti_req_create(
            to_node->stream,
            pkg,
            TI_PROTO_NODE_REQ_FWD_TIMEOUT,
            api__fwd_cb,
            ar))
    {
        ti_api_release(ar);
        return -1;
    }

    return 0;
}

static int api__run(ti_api_request_t * ar, api__req_t * req)
{
    ti_pkg_t * pkg;
    vec_t * access_;
    ti_query_t * query = NULL;
    ti_node_t * this_node = ti()->node, * other_node;
    ex_t * e = &ar->e;
    unsigned char * data = NULL;
    size_t n;

    if (req->mp_procedure.tp != MP_STR)
        goto invalid_api_request;

    if (this_node->status < TI_NODE_STAT_READY &&
        this_node->status != TI_NODE_STAT_SHUTTING_DOWN)
    {
        other_node = ti_nodes_random_ready_node();
        if (!other_node)
        {
            ti_nodes_set_not_ready_err(e);
            return e->nr;
        }

        pkg = api__gen_fwd_pkg(ar, req, api__gen_run_data, TI_PROTO_NODE_REQ_RUN);
        if (!pkg || api__fwd(ar, other_node, pkg))
        {
            free(pkg);
            ex_set_internal(e);
            return e->nr;
        }

        /* the response to the client will be handled by a callback on the
         * query forward request so we simply return;
         */
        return 0;
    }

    query = ti_query_create(ar, ar->user, TI_QBIND_FLAG_API);

    if (!query || api__gen_run_data(ar, req, &data, &n))
    {
        ex_set_mem(e);
        goto failed;
    }

    if (ti_query_unp_run(query, &ar->scope, 0, data, n, &ar->e))
        goto failed;

    free(data);

    access_ = ti_query_access(query);
    assert (access_);

    if (ti_access_check_err(access_, query->user, TI_AUTH_RUN, e))
        goto failed;

    if (ti_query_will_update(query))
    {
        if (ti_access_check_err(access_, query->user, TI_AUTH_MODIFY, e) ||
            ti_events_create_new_event(query, e))
            goto failed;

        /* cleanup will be done by the event */
        return 0;
    }

    ti_query_run(query);
    return 0;

invalid_api_request:
    ex_set(e, EX_BAD_DATA, "invalid API request"DOC_HTTP_API);
failed:
    ti_query_destroy(query);
    free(data);
    return e->nr;
}

static int api__query(ti_api_request_t * ar, api__req_t * req)
{
    ti_pkg_t * pkg;
    vec_t * access_;
    ti_query_t * query = NULL;
    ti_node_t * this_node = ti()->node, * other_node;
    ex_t * e = &ar->e;

    if (req->mp_code.tp != MP_STR)
        goto invalid_api_request;

    if (ar->scope.tp == TI_SCOPE_NODE)
    {
        if (ar->scope.via.node_id == this_node->id)
            goto query;

        other_node = ti_nodes_node_by_id(ar->scope.via.node_id);
        if (!other_node)
        {
            ex_set(e, EX_LOOKUP_ERROR, TI_NODE_ID" does not exist",
                    ar->scope.via.node_id);
            return e->nr;
        }

        if (other_node->status <= TI_NODE_STAT_BUILDING)
        {
            ex_set(e, EX_LOOKUP_ERROR,
                    TI_NODE_ID" is not able to handle this request",
                    ar->scope.via.node_id);
            return e->nr;
        }

        pkg = api__gen_fwd_pkg(ar, req, api__gen_query_data, TI_PROTO_NODE_REQ_QUERY);
        if (!pkg || api__fwd(ar, other_node, pkg))
        {
            free(pkg);
            ex_set_internal(e);
            return e->nr;
        }
        /* the response to the client will be handled by a callback on the
         * query forward request so we simply return;
         */
        return 0;
    }

    if (this_node->status < TI_NODE_STAT_READY &&
        this_node->status != TI_NODE_STAT_SHUTTING_DOWN)
    {
        other_node = ti_nodes_random_ready_node();
        if (!other_node)
        {
            ti_nodes_set_not_ready_err(e);
            return e->nr;
        }

        pkg = api__gen_fwd_pkg(ar, req, api__gen_query_data, TI_PROTO_NODE_REQ_QUERY);
        if (!pkg || api__fwd(ar, other_node, pkg))
        {
            free(pkg);
            ex_set_internal(e);
            return e->nr;
        }

        /* the response to the client will be handled by a callback on the
         * query forward request so we simply return;
         */
        return 0;
    }

query:
    query = ti_query_create(ar, ar->user, TI_QBIND_FLAG_API);

    if (!query || !(query->querystr = mp_strdup(&req->mp_code)))
    {
        ex_set_mem(e);
        goto failed;
    }

    if (ti_query_apply_scope(query, &ar->scope, e))
        goto failed;

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
            goto failed;
    }

    access_ = ti_query_access(query);
    assert (access_);

    if (ti_access_check_err(access_, query->user, TI_AUTH_READ, e) ||
        ti_query_parse(query, e) ||
        ti_query_investigate(query, e))
        goto failed;

    if (ti_query_will_update(query))
    {
        assert (ar->scope.tp != TI_SCOPE_NODE);

        if (ti_access_check_err(access_, query->user, TI_AUTH_MODIFY, e) ||
            ti_events_create_new_event(query, e))
            goto failed;

        /* cleanup will be done by the event */
        return 0;
    }

    ti_query_run(query);
    return 0;

invalid_api_request:
    ex_set(e, EX_BAD_DATA, "invalid API request"DOC_HTTP_API);
failed:
    ti_query_destroy(query);
    return e->nr;
}

static int api__from_msgpack(ti_api_request_t * ar)
{
    api__req_t request = {0};
    mp_unp_t up;
    mp_obj_t obj, mp_key;
    size_t i;

    mp_unp_init(&up, ar->content, ar->content_n);

    if (mp_next(&up, &obj) != MP_MAP || obj.via.sz < 2)
        goto invalid_api_request;

    for (i = obj.via.sz; i--;)
    {
        if (mp_next(&up, &mp_key) != MP_STR)
            goto invalid_api_request;

        if (mp_str_eq(&mp_key, "type"))
            mp_next(&up, &request.mp_type);

        else if (mp_str_eq(&mp_key, "code"))
            mp_next(&up, &request.mp_code);

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
        if (api__query(ar, &request))
            goto failed;
        return 0;
    }

    if (mp_str_eq(&request.mp_type, "run"))
    {
        if (api__run(ar, &request))
            goto failed;
        return 0;
    }

invalid_api_request:
    ex_set(&ar->e, EX_BAD_DATA, "invalid API request"DOC_HTTP_API);
failed:
    ++ti()->counters->queries_with_error;
    ti_api_close_with_err(ar, &ar->e);

    return 0;
}

static int api__plain_response(ti_api_request_t * ar, const api__header_t ht)
{
    const char * body = api__default_body[ht];
    char header[API__HEADER_MAX_SZ];
    size_t body_size;
    int header_size;

    body_size = strlen(body);
    header_size = api__header(header, ht, TI_API_CT_TEXT, body_size);
    if (header_size > 0)
    {
        uv_buf_t uvbufs[2] = {
                uv_buf_init(header, (size_t) header_size),
                uv_buf_init((char *) body, body_size),
        };

        (void) uv_write(
                &ar->req,
                &ar->uvstream,
                uvbufs, 2,
                api__write_cb);
        return 0;
    }
    return -1;
}

static int api__message_complete_cb(http_parser * parser)
{
    ti_api_request_t * ar = parser->data;

    if (parser->method != HTTP_POST)
        return api__plain_response(ar, E405_METHOD_NOT_ALLOWED);

    if (ar->flags & TI_API_FLAG_INVALID_SCOPE)
        return api__plain_response(ar, E404_NOT_FOUND);

    if (!ar->user)
        return api__plain_response(ar, E401_UNAUTHORIZED);

    switch (ar->content_type)
    {
    case TI_API_CT_TEXT:
        return api__plain_response(ar, E415_UNSUPPORTED_MEDIA_TYPE);
    case TI_API_CT_JSON:
    {
        char * data;
        size_t size;
        if (mpjson_json_to_mp(ar->content, ar->content_n, &data, &size))
            return api__plain_response(ar, E400_BAD_REQUEST);

        free(ar->content);
        ar->content = data;
        ar->content_n = size;
    }
    /* fall through */
    case TI_API_CT_MSGPACK:
        assert (ar->e.nr == 0);
        return api__from_msgpack(ar);
    }

    return api__plain_response(ar, E500_INTERNAL_SERVER_ERROR);
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

ti_api_request_t * ti_api_acquire(ti_api_request_t * ar)
{
    ar->flags |= TI_API_FLAG_IN_USE;
    return ar;
}

void ti_api_release(ti_api_request_t * ar)
{
    ar->flags &= ~TI_API_FLAG_IN_USE;

    if (ar->flags & TI_API_FLAG_IS_CLOSED)
        uv_close((uv_handle_t *) &ar->uvstream, api__close_cb);
}

void ti_api_close(ti_api_request_t * ar)
{
    if (!ar || (ar->flags & TI_API_FLAG_IS_CLOSED))
        return;

    ar->flags |= TI_API_FLAG_IS_CLOSED;

    if (ar->flags & TI_API_FLAG_IN_USE)
        return;

    uv_close((uv_handle_t *) &ar->uvstream, api__close_cb);
}


