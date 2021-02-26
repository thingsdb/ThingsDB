/*
 * ti/timers.c
 */
#include <assert.h>
#include <ti/timers.h>
#include <ti/timer.h>
#include <ti/timer.inline.h>
#include <stdbool.h>
#include <ti/node.h>
#include <ti.h>

/* check two times within the minimal repeat value */
#define TIMERS__INTERVAL ((TI_TIMERS_MIN_REPEAT*1000)/2)

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
    timers->timers = vec_new(4);

    if (!timers->timer || !timers->timers)
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

void ti_timers_del_user(ti_user_t * user)
{
    for (vec_each(ti.timers->timers, ti_timer_t, timer))
        if (timer->user == user)
            ti_timer_mark_del(timer);

    for (vec_each(ti.collections->vec, ti_collection_t, collection))
        for (vec_each(collection->timers, ti_timer_t, timer))
            ti_timer_mark_del(timer);
}

vec_t ** ti_timers_from_scope_id(uint64_t scope_id)
{
    if (scope_id == TI_SCOPE_THINGSDB)
        return &ti.timers->timers;

    for (vec_each(ti.collections->vec, ti_collection_t, collection))
        if (collection->root->id == scope_id)
            return &collection->timers;

    return NULL;
}

static void timers__destroy(uv_handle_t * UNUSED(handle))
{
    if (timers)
    {
        free(timers->timer);
        vec_destroy(timers->timers, (vec_destroy_cb) ti_timer_drop);
    }
    free(timers);
    timers = ti.timers = NULL;
}

static int timer__handle(vec_t * timers, uint64_t now)
{
    int n = 0;
    size_t i = timers->n;
    uint32_t rel_id = ti.rel_id;
    uint32_t nodes_n = ti.nodes->vec->n;

    for (vec_each_rev(timers, ti_timer_t, timer))
    {
        --i;

        if (!timer->user)
        {
            ti_timer_drop(vec_swap_remove(ti.timers->timers, i));
            continue;
        }

        if (timer->next_run > now)
            continue;

        if (timer->id % nodes_n == rel_id)
        {
            if (timer->ref == 1)
            {
                ++n;
                cb(timer);
            }
            continue;
        }

        if (!timer->repeat && ((now - timer->next_run) > 120))
        {
            ti_timer_drop(vec_swap_remove(ti.timers->timers, i));
        }
    }

    return n;
}

/*
 * Called from the main thread at CONNECT__INTERVAL (every X seconds)
 */
static void timers__cb(uv_timer_t * UNUSED(handle))
{
    uint64_t now = util_now_usec();
    ti_timers_cb cb;
    size_t i;
    int n;

    if ((ti.node->status & (
            TI_NODE_STAT_SYNCHRONIZING|
            TI_NODE_STAT_AWAY|
            TI_NODE_STAT_AWAY_SOON|
            TI_NODE_STAT_READY)) == 0)
        return;

    cb = ti.node->status == TI_NODE_STAT_READY ? ti_timer_run : ti_timer_fwd;

    n = timer__handle(ti.timers->timers, now);

    for (vec_each(ti.collections->vec, ti_collection_t, collection))
        n += timer__handle(collection->timers, now);

    log_debug(
            "%s %zu timer%s",
            cb == ti_timer_run ? "handled" : "forwarded", n, n == 1 ? "": "s");
}

