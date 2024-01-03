/*
 * ti/tasks.c
 */
#include <assert.h>
#include <stdbool.h>
#include <ti.h>
#include <ti/collection.h>
#include <ti/node.h>
#include <ti/restore.h>
#include <ti/tasks.h>
#include <ti/val.inline.h>
#include <ti/varr.h>
#include <ti/vtask.h>
#include <ti/vtask.inline.h>
#include <util/fx.h>

/* Tasks will be checked every X seconds */
#define VTASKS__INTERVAL 1 * 1000

static ti_tasks_t * tasks;

static void tasks__destroy(uv_handle_t * UNUSED(handle));
static void tasks__cb(uv_timer_t * UNUSED(handle));

int ti_tasks_create(void)
{
    tasks = malloc(sizeof(ti_tasks_t));
    if (!tasks)
        goto failed;

    tasks->is_stopping = false;
    tasks->is_started = false;
    tasks->lowest_run_at = 0;
    tasks->timer = malloc(sizeof(uv_timer_t));
    tasks->vtasks = vec_new(4);

    if (!tasks->timer || !tasks->vtasks)
        goto failed;

    ti.tasks = tasks;
    return 0;

failed:
    tasks__destroy(NULL);
    ti.tasks = NULL;
    return -1;
}

int ti_tasks_start(void)
{
    assert(tasks->is_started == false);

    if (uv_timer_init(ti.loop, tasks->timer) ||
        uv_timer_start(
            tasks->timer,
            tasks__cb,
            VTASKS__INTERVAL,       /* start at VTASKS__INTERVAL */
            VTASKS__INTERVAL))      /* repeat at VTASKS__INTERVAL */
        goto fail;

    tasks->is_started = true;
    return 0;

fail:
    uv_close((uv_handle_t *) tasks->timer, tasks__destroy);
    return -1;
}

void ti_tasks_stop(void)
{
    if (!tasks || tasks->is_stopping)
        return;

    tasks->is_stopping = true;

    if (!tasks->is_started)
    {
        tasks__destroy(NULL);
    }
    else
    {
        tasks->is_started = false;
        uv_timer_stop(tasks->timer);
        uv_close((uv_handle_t *) tasks->timer, tasks__destroy);
    }
}

static void tasks__destroy(uv_handle_t * UNUSED(handle))
{
    if (tasks)
    {
        free(tasks->timer);
        vec_destroy(tasks->vtasks, (vec_destroy_cb) ti_vtask_drop);
    }
    free(tasks);
    tasks = ti.tasks = NULL;
}

static inline int tasks__handle_vtask(
        ti_vtask_t * vtask,
        ti_collection_t * collection,
        const uint64_t now,
        const uint32_t rel_id,
        const uint32_t nodes_n)
{
    if (vtask->run_at &&
        (vtask->id % nodes_n == rel_id) &&
        (~vtask->flags & TI_VTASK_FLAG_RUNNING))
    {
        if (vtask->run_at <= now)
        {
            if (ti_vtask_run(vtask, collection) == 0)
                return 1;
            /* re-schedule on error */
        }

        /* update the next lowest run */
        if (vtask->run_at < tasks->lowest_run_at)
            tasks->lowest_run_at = vtask->run_at;
    }
    return 0;
}

/*
 * Called from the main thread at CONNECT__INTERVAL (every X seconds)
 */
static void tasks__cb(uv_timer_t * UNUSED(handle))
{
    const uint64_t now = util_now_usec();
    const uint32_t rel_id = ti.rel_id;
    const uint32_t nodes_n = ti.nodes->vec->n;
    int n = 0;

    /* ThingsDB knows the first time-stamp it should handle, thus this call is
     * usually very sort as it will return immediately. Only when required, it
     * will loop through the tasks.
     */
    if (tasks->lowest_run_at > now || ti_restore_is_busy() || (
        ti.node->status & (
            TI_NODE_STAT_AWAY|
            TI_NODE_STAT_AWAY_SOON|
            TI_NODE_STAT_READY)) == 0)
        return;

    tasks->lowest_run_at = UINT64_MAX;

    for (vec_each(tasks->vtasks, ti_vtask_t, vtask))
        n += tasks__handle_vtask(vtask, NULL, now, rel_id, nodes_n);

    for (vec_each(ti.collections->vec, ti_collection_t, collection))
        for (vec_each(collection->vtasks, ti_vtask_t, vtask))
            n += tasks__handle_vtask(vtask, collection, now, rel_id, nodes_n);

    if (n)
        log_info("initiated %d task%s", n, n == 1 ? "": "s");
}

int ti_tasks_append(vec_t ** vtasks, ti_vtask_t * vtask)
{
    uint32_t rel_id = ti.rel_id;
    uint32_t nodes_n = ti.nodes->vec->n;

    if (vec_push(vtasks, vtask))
        return -1;

    if ((vtask->id % nodes_n == rel_id) &&
        vtask->run_at < tasks->lowest_run_at)
        tasks->lowest_run_at = vtask->run_at;

    return 0;
}
/*
 * Only for dropped collections.
 */
void ti_tasks_clear_dropped(vec_t ** vtasks)
{
    ti_vtask_t * vtask;
    while ((vtask = vec_pop(*vtasks)))
    {
        ti_vtask_unsafe_drop(vtask);
    }
    vec_shrink(vtasks);
}

void ti_tasks_del_user(ti_user_t * user)
{
    for (vec_each_rev(tasks->vtasks, ti_vtask_t, vtask))
        if (vtask->user == user)
            ti_vtask_del(vtask->id, NULL);

    for (vec_each(ti.collections->vec, ti_collection_t, collection))
        for (vec_each(collection->vtasks, ti_vtask_t, vtask))
            if (vtask->user == user)
                ti_vtask_del(vtask->id, collection);
}

void ti_tasks_clear_all(void)
{
    for (vec_each_rev(tasks->vtasks, ti_vtask_t, vtask))
        ti_vtask_del(vtask->id, NULL);

    for (vec_each(ti.collections->vec, ti_collection_t, collection))
        ti_collection_tasks_clear(collection);
}

ti_varr_t * ti_tasks_list(vec_t * tasks)
{
    ti_varr_t * varr = ti_varr_create(tasks->n);
    if (!varr)
        return NULL;

    for (vec_each(tasks, ti_vtask_t, vtask))
    {
        VEC_push(varr->vec, vtask);
        ti_incref(vtask);
    }

    return varr;
}

vec_t * ti_tasks_from_scope_id(uint64_t scope_id)
{
    if (scope_id == TI_SCOPE_THINGSDB)
        return tasks->vtasks;

    for (vec_each(ti.collections->vec, ti_collection_t, collection))
        if (collection->id == scope_id)
            return collection->vtasks;

    return NULL;
}

void ti_tasks_reset_lowest_run_at(void)
{
    tasks->lowest_run_at = 0;  /* forces a one time loop */
}

void ti_tasks_vtask_finish(ti_vtask_t * vtask)
{
    if (vtask->run_at)
    {
        uint32_t rel_id = ti.rel_id;
        uint32_t nodes_n = ti.nodes->vec->n;

        if ((vtask->id % nodes_n == rel_id) &&
            vtask->run_at < tasks->lowest_run_at)
            tasks->lowest_run_at = vtask->run_at;
    }
    vtask->flags &= ~(TI_VTASK_FLAG_RUNNING|TI_VTASK_FLAG_AGAIN);
}

