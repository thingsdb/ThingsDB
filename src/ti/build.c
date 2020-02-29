/*
 * ti/build.c
 */
#include <assert.h>
#include <ex.h>
#include <ti.h>
#include <ti/api.h>
#include <ti/build.h>
#include <ti/node.h>
#include <ti/pkg.h>
#include <ti/proto.h>
#include <ti/req.h>
#include <ti/sync.h>
#include <ti/version.h>
#include <util/logger.h>

static ti_build_t * build;

static void build__on_setup_cb(ti_req_t * req, ex_enum status);


int ti_build_create(void)
{
    build = malloc(sizeof(ti_build_t));
    if (!build)
        return -1;

    build->status = TI_BUILD_WAITING;

    ti()->build = build;
    return 0;
}

void ti_build_destroy(void)
{
    if (!build)
        return;
    free(build);
    ti()->build = build = NULL;
}

int ti_build_setup(
        uint32_t this_node_id,
        uint32_t from_node_id,
        uint8_t from_node_status,
        uint8_t from_node_zone,
        uint16_t from_node_syntax_ver,
        uint16_t from_node_port,
        ti_stream_t * stream)
{
    assert (build->status == TI_BUILD_WAITING);

    ti_pkg_t * pkg;

    if (ti_write_node_id(&this_node_id))
        return -1;

    pkg = ti_pkg_new(0, TI_PROTO_NODE_REQ_SETUP, NULL, 0);
    if (!pkg)
    {
        log_critical(EX_MEMORY_S);
        return -1;
    }

    build->status = TI_BUILD_REQ_SETUP;
    build->this_node_id = this_node_id;
    build->from_node_id = from_node_id;
    build->from_node_status = from_node_status;
    build->from_node_zone = from_node_zone;
    build->from_node_syntax_ver = from_node_syntax_ver;
    build->from_node_port = from_node_port;

    if (ti_req_create(
            stream,
            pkg,
            TI_PROTO_NODE_REQ_SETUP_TIMEOUT,
            build__on_setup_cb,
            stream))
    {
        free(pkg);
        build->status = TI_BUILD_WAITING;
        log_error(EX_INTERNAL_S);
        return -1;
    }

    return 0;
}

static void build__on_setup_cb(ti_req_t * req, ex_enum status)
{
    ti_pkg_t * pkg = req->pkg_res;
    ti_node_t * ti_node;
    vec_t * nodes_vec;

    if (status)
        goto failed;

    if (pkg->tp != TI_PROTO_NODE_RES_SETUP)
    {
        ti_pkg_log(pkg);
        goto failed;
    }

    if (ti_unpack(pkg->data, pkg->n))
        goto failed;

    /* assign nodes_vec not earlier since ti_unpack might change the pointer */
    nodes_vec = ti()->nodes->vec;
    for(vec_each(nodes_vec, ti_node_t, node))
    {
        if (node->id == build->from_node_id)
        {
            ti_stream_set_node(req->stream, node);
            ti_node_upd_port(node, build->from_node_port);

            node->status = build->from_node_status;
            node->zone = build->from_node_zone;
            node->syntax_ver = build->from_node_syntax_ver;
        }
    }

    ti_node = ti()->node;

    ti_node->cevid = 0;
    ti_node->sevid = 0;
    ti_node->next_thing_id = 0;
    ti_node->syntax_ver = TI_VERSION_SYNTAX;
    ti_node->status = TI_NODE_STAT_SYNCHRONIZING;

    ti_nodes_update_syntax_ver(TI_VERSION_SYNTAX);

    if (ti_save())
        goto failed;

    if (ti_store_store())
        goto failed;

    if (ti_archive_init())
        goto failed;

    if (ti_nodes_write_global_status())
        goto failed;

    if (ti_away_start())
        goto failed;

    if (ti_clients_listen())
        goto failed;

    if (ti_api_init())
        goto failed;

    if (ti_connect_start())
        goto failed;

    if (ti_sync_start())
        goto failed;

    ti_broadcast_node_info();

    goto done;

failed:
    build->status = TI_BUILD_WAITING;

done:
    ti_req_destroy(req);
}
