/*
 * ti/build.c
 */
#include <assert.h>
#include <ti/build.h>
#include <ti/node.h>
#include <ti/pkg.h>
#include <ti/proto.h>
#include <ti/req.h>
#include <ti/ex.h>
#include <ti.h>
#include <util/logger.h>

static ti_build_t * build;

static void build__on_setup_cb(ti_req_t * req, ex_enum status);


int ti_build_create(void)
{
    build = malloc(sizeof(ti_build_t));
    if (!build)
        return -1;

    build->streams = omap_create();
    build->status = TI_BUILD_WAITING;
    build->req_setup_pkg = ti_pkg_new(0, TI_PROTO_NODE_REQ_SETUP, NULL, 0);

    if (!build->streams || build->req_setup_pkg)
    {
        ti_build_destroy();
        return -1;
    }

    ti()->build = build;
    return 0;
}

void ti_build_destroy(void)
{
    if (!build)
        return;
    omap_destroy(build->streams, (omap_destroy_cb) ti_stream_drop);
    free(build->req_setup_pkg);
    free(build);
    ti()->build = build = NULL;
}

/*
 * Returns 0 if successful, < 0 if writing the node_id has failed or > 0 when
 * the node_id does not correspond with an earlier received node_id.
 */
int ti_build_set_node_id(uint8_t node_id)
{
    if (build->status >= TI_BUILD_SET_NODE_ID)
        return build->node_id == node_id ? 0 : 1;

    build->status = TI_BUILD_SET_NODE_ID;
    build->node_id = node_id;

    return ti_write_node_id(&node_id);
}

int ti_build_setup(uint8_t node_id, ti_stream_t * stream)
{
    assert (node_id != build->node_id);

    int rc;

    if (build->status == TI_BUILD_REQ_SETUP)
        return 0;

    rc = omap_add(build->streams, node_id, stream);
    if (rc == OMAP_ERR_ALLOC)
    {
        log_critical(EX_ALLOC_S);
        return -1;
    }

    ti_incref(stream);

    if (rc == OMAP_ERR_EXIST)
    {
        ti_stream_t * prev = omap_get(build->streams, node_id);
        if (prev == stream)
        {
            ti_decref(stream);
        }
        else
        {
            if (omap_set(build->streams, node_id, stream))
            {
                log_critical(EX_ALLOC_S);
                return -1;
            }

            log_warning(
                    "build queue has replaced stream "
                    "`%s` with `%s` for "TI_NODE_ID,
                    ti_stream_name(prev),
                    ti_stream_name(stream),
                    node_id);
            ti_stream_drop(prev);
        }
    }

    if (ti_req_create(
            stream,
            build->req_setup_pkg,
            TI_PROTO_NODE_REQ_SETUP_TIMEOUT,
            build__on_setup_cb,
            stream))
    {
        log_critical(EX_ALLOC_S);
        return -1;
    }

    build->status = TI_BUILD_REQ_SETUP;
    build->req_node_id = node_id;

    return 0;
}

static void build__on_setup_cb(ti_req_t * req, ex_enum status)
{
    ti_stream_t * stream = req->data;
    ti_pkg_t * pkg = req->pkg_res;

    if (status)
        goto failed;

    if (ti_unpack(pkg->data, pkg->n))
        goto failed;

    for(vec_each(ti()->nodes->vec, ti_node_t, node))
    {
        if (node->id == build->req_node_id)
        {
            int len = sizeof(node->addr);
            if (uv_tcp_getpeername(
                    (uv_tcp_t *) stream,
                    (struct sockaddr *) &node->addr,
                    &len))
            {
                log_warning(
                        "cannot replace socket address for "TI_NODE_ID,
                        node->id);
            }
        }
    }

    ti_build_destroy();
    /* we should now start with synchronizing */

    goto done;

failed:
    build->status = TI_BUILD_SET_NODE_ID;
done:
    ti_req_destroy(req);
}
