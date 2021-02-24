/*
 * ti/timers.c
 */
#include <assert.h>
#include <ti/timers.h>
#include <stdbool.h>
#include <ti/node.h>
#include <ti.h>

/* 9 seconds */
#define TIMERS__INTERVAL 9000

static ti_timers_t * timers;

static void timers__destroy(uv_handle_t * UNUSED(handle));
static void timers__cb(uv_timer_t * UNUSED(handle));

int ti_timers_create(void)
{
    timers = malloc(sizeof(ti_timers_t));
    if (!timers)
        goto failed;

    timers->is_started = false;
    timers->timer = malloc(sizeof(uv_timer_t));
    timers->n_loops = 0;

    if (!timers->timer)
        goto failed;

    ti.timers = timers;
    return 0;

failed:
    timers__destroy(NULL);
    ti.timers = NULL;
    return -1;
}

int ti_timers_start(void)
{
    assert (timers->is_started == false);

    if (uv_timer_init(ti.loop, timers->timer))
        goto fail0;

    if (uv_timer_start(
            timers->timer,
            timers__cb,
            5000,                   /* start after 5 seconds */
            TIMERS__INTERVAL))      /* repeat at TIMERS__INTERVAL */
        goto fail1;

    timers->is_started = true;
    return 0;

fail1:
    uv_close((uv_handle_t *) timers->timer, timers__destroy);
fail0:
    return -1;
}

void ti_timers_stop(void)
{
    if (!timers)
        return;

    if (!timers->is_started)
        timers__destroy(NULL);
    else
    {
        uv_timer_stop(timers->timer);
        uv_close((uv_handle_t *) timers->timer, timers__destroy);
    }
}

static void timers__destroy(uv_handle_t * UNUSED(handle))
{
    if (timers)
        free(timers->timer);
    free(timers);
    timers = ti.timers = NULL;
}

/*
 * Called from the main thread at CONNECT__INTERVAL (every X seconds)
 */
static void timers__cb(uv_timer_t * UNUSED(handle))
{
    uint64_t now = util_now_tsec();
    uint32_t rel_id = ti.rel_id;
    uint32_t nodes_n = ti.nodes->vec->n;
    ti_timers_cb cb;

    if (ti.node->status & (
            TI_NODE_STAT_SYNCHRONIZING|
            TI_NODE_STAT_AWAY|
            TI_NODE_STAT_AWAY_SOON|
            TI_NODE_STAT_READY) == 0)
        return;

    cb = ti.node->status == TI_NODE_STAT_READY ? ti_timer_run : ti_timer_fwd;

    for (vec_each(ti.timers_node, ti_timer_t, timer))
    {
        if (timer->next_run <= now)
            ti_timer_run(timer);
    }

    for (vec_each(ti.timers_thingsdb, ti_timer_t, timer))
    {
        if (timer->next_run <= now && (timer->id % nodes_n == rel_id))
            cb(timer);
    }

    for (vec_each(ti.collections->vec, ti_collection_t, collection))
    {
        for (vec_each(collection->timers, ti_timer_t, timer))
            if (timer->next_run <= now || (timer->id % nodes_n == rel_id))
                cb(timer);
    }
}

