/*
 * ti/connect.c
 */
#include <assert.h>
#include <ti/connect.h>
#include <stdbool.h>
#include <ti/node.h>
#include <ti.h>

static ti_connect_t * connect_loop;

static void connect__destroy(uv_handle_t * UNUSED(handle));
static void connect__cb(uv_timer_t * UNUSED(handle));

int ti_connect_create(void)
{
    connect_loop = malloc(sizeof(ti_connect_t));
    if (!connect_loop)
        goto failed;

    connect_loop->is_started = false;
    connect_loop->interval = 2000;                      /* 2 seconds */
    connect_loop->timer = malloc(sizeof(uv_timer_t));
    connect_loop->n_loops = 0;

    if (!connect_loop->timer)
        goto failed;

    ti()->connect_loop = connect_loop;
    return 0;

failed:
    connect__destroy(NULL);
    ti()->connect_loop = NULL;
    return -1;
}

int ti_connect_start(void)
{
    assert (connect_loop->is_started == false);

    if (uv_timer_init(ti()->loop, connect_loop->timer))
        goto fail0;

    if (uv_timer_start(
            connect_loop->timer,
            connect__cb,
            1000,
            connect_loop->interval))
        goto fail1;

    connect_loop->is_started = true;
    return 0;

fail1:
    uv_close((uv_handle_t *) connect_loop->timer, connect__destroy);
fail0:
    return -1;
}

void ti_connect_stop(void)
{
    if (!connect_loop)
        return;

    if (!connect_loop->is_started)
        connect__destroy(NULL);
    else
    {
        uv_timer_stop(connect_loop->timer);
        uv_close((uv_handle_t *) connect_loop->timer, connect__destroy);
    }
}

static void connect__destroy(uv_handle_t * UNUSED(handle))
{
    if (connect_loop)
        free(connect_loop->timer);
    free(connect_loop);
    connect_loop = ti()->connect_loop = NULL;
}


static void connect__cb(uv_timer_t * UNUSED(handle))
{
    uint32_t n = ++connect_loop->n_loops;
    ti_rpkg_t * rpkg = NULL;


    for (vec_each(ti()->nodes->vec, ti_node_t, node))
    {
        uint32_t step;
        if (node == ti()->node)
            continue;

        if (node->status == TI_NODE_STAT_OFFLINE)
        {
            if (n < node->next_retry)
                continue;

            ++node->retry_counter;
            step = node->retry_counter * 2;
            /* max step will be 60 * 2 seconds */
            node->next_retry = n + (step < 60 ? step : 60);

            if (!node->stream && ti_node_connect(node))
                log_error(EX_INTERNAL_S);
        }
        else if (
            !((n + node->id) % 15) &&
            (   node->status == TI_NODE_STAT_SYNCHRONIZING ||
                node->status == TI_NODE_STAT_AWAY ||
                node->status == TI_NODE_STAT_AWAY_SOON ||
                node->status == TI_NODE_STAT_READY))
        {
            rpkg = rpkg ? rpkg : ti_node_status_rpkg();
            if (!rpkg)
                return;

            /* every 15 loops of 2 seconds we write our status */
            if (ti_stream_write_rpkg(node->stream, rpkg))
                log_error(EX_INTERNAL_S);
        }
    }
    ti_rpkg_drop(rpkg);
}
