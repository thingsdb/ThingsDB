/*
 * nodes.c
 */
#include <nodes.h>
#include <thingsdb.h>
#include <ti/node.h>
#include <stdbool.h>

static thingsdb_nodes_t * nodes;


int thingsdb_nodes_create(void)
{
    nodes = malloc(sizeof(thingsdb_nodes_t));
    if (!nodes)
        return -1;

    nodes->vec = vec_new(0);
    thingsdb_get()->nodes = nodes;

    return -(nodes == NULL);
}

void thingsdb_nodes_destroy(void)
{
    vec_destroy(nodes->vec, (vec_destroy_cb) ti_node_drop);
    free(nodes);
    nodes = thingsdb_get()->nodes = NULL;
}

_Bool thingsdb_nodes_has_quorum(void)
{
    size_t quorum = (nodes->vec->n + 1) / 2;
    size_t q = 0;

    for (vec_each(nodes->vec, ti_node_t, node))
    {
        if (node->status > TI_NODE_STAT_CONNECTED && ++q == quorum) return 1;
    }

    return 0;
}

int thingsdb_nodes_listen(void)
{
    int rc;
    thingsdb_t * thingsdb = thingsdb_get();
    ti_cfg_t * cfg = thingsdb->cfg;
    struct sockaddr_storage addr;
    _Bool is_ipv6 = false;
    char * ip;

    uv_tcp_init(thingsdb->loop, &nodes->tcp);
    uv_pipe_init(thingsdb->loop, &nodes->pipe, 0);

    if (cfg->bind_node_addr != NULL)
    {
        struct in6_addr sa6;
        if (inet_pton(AF_INET6, cfg->bind_node_addr, &sa6))
        {
            is_ipv6 = true;
        }
        ip = cfg->bind_client_addr;
    }
    else if (cfg->ip_support == AF_INET)
    {
        ip = "0.0.0.0";
    }
    else
    {
        ip = "::";
        is_ipv6 = true;
    }

    if (is_ipv6)
    {
        uv_ip6_addr(ip, cfg->node_port, (struct sockaddr_in6 *) &addr);
    }
    else
    {
        uv_ip4_addr(ip, cfg->node_port, (struct sockaddr_in *) &addr);
    }

    if ((rc = uv_tcp_bind(
            &nodes->tcp,
            (const struct sockaddr *) &addr,
            (cfg->ip_support == AF_INET6) ?
                    UV_TCP_IPV6ONLY : 0)) ||
        (rc = uv_listen(
            (uv_stream_t *) &nodes->tcp,
            THINGSDB_MAX_NODES,
            thingsdb__nodes_tcp_connection)))
    {
        log_error("error listening for TCP nodes: `%s`", uv_strerror(rc));
        return -1;
    }

    log_info("start listening for TCP nodes on port %d", cfg->node_port);

    if (!cfg->pipe_support)
        return 0;

    if ((rc = uv_pipe_bind(&nodes->pipe, cfg->pipe_node_name)) ||
        (rc = uv_listen(
                (uv_stream_t *) &nodes->pipe,
                THINGSDB_MAX_NODES,
                thingsdb__nodes_pipe_connection)))
    {
        log_error("error listening for PIPE nodes: `%s`", uv_strerror(rc));
        return -1;
    }

    log_info("start listening for PIPE nodes connections on `%s`",
            cfg->pipe_node_name);

    return 0;
}


static void thingsdb__nodes_tcp_connection(uv_stream_t * uvstream, int status)
{
    ti_stream_t * stream;

    if (status < 0)
    {
        log_error("node connection error: `%s`", uv_strerror(status));
        return;
    }

    log_debug("received a TCP node connection");

    stream = ti_stream_create(TI_STREAM_TCP_IN_NODE, &thingsdb__nodes_pkg_cb);

    if (!stream)
        return;

    uv_tcp_init(thingsdb_get()->loop, (uv_tcp_t *) stream->uvstream);
    if (uv_accept(uvstream, stream->uvstream) == 0)
    {
        uv_read_start(stream->uvstream, ti_stream_alloc_buf, ti_stream_on_data);
    }
    else
    {
        ti_stream_drop(stream);
    }
}

static void thingsdb__nodes_pipe_connection(uv_stream_t * uvstream, int status)
{
    ti_stream_t * stream;

    if (status < 0)
    {
        log_error("node connection error: `%s`", uv_strerror(status));
        return;
    }

    log_debug("received a PIPE node connection");

    stream = ti_stream_create(TI_STREAM_PIPE_IN_NODE, &thingsdb__nodes_pkg_cb);

    if (!stream)
        return;

    uv_pipe_init(thingsdb_get()->loop, (uv_pipe_t *) stream->uvstream, 0);
    if (uv_accept(uvstream, stream->uvstream) == 0)
    {
        uv_read_start(stream->uvstream, ti_stream_alloc_buf, ti_stream_on_data);
    }
    else
    {
        ti_stream_drop(stream);
    }
}

static void thingsdb__nodes_pkg_cb(ti_stream_t * stream, ti_pkg_t * pkg)
{

}
