/*
 * ti/timers.c
 */
#include <assert.h>
#include <stdbool.h>
#include <ti/node.h>
#include <ti/timer.h>
#include <ti/timer.inline.h>
#include <ti/timers.h>
#include <ti/val.inline.h>
#include <ti/varr.h>
#include <ti.h>
#include <util/fx.h>

/* check ten times within the minimal repeat value */
#define TIMERS__INTERVAL ((TI_TIMERS_MIN_REPEAT*1000)/10)

/*
 * non-repeating timers which are not handled by this node will expire after X
 * seconds
 */
#define TIMERS__EXPIRE_TIME 3600

static ti_timers_t * timers;

static void timers__destroy(uv_handle_t * UNUSED(handle));
static void timers__cb(uv_timer_t * UNUSED(handle));

static const char * timers__stat_fn = "timers_stat.mp";
static char * timers__get_fn(void)
{
    if (!timers->stat_fn)
        timers->stat_fn = fx_path_join(ti.cfg->storage_path, timers__stat_fn);

    return timers->stat_fn;
}

int ti_timers_create(void)
{
    timers = malloc(sizeof(ti_timers_t));
    if (!timers)
        goto failed;

    timers->is_stopping = false;
    timers->is_started = false;
    timers->timer = malloc(sizeof(uv_timer_t));
    timers->n_loops = 0;
    timers->timers = vec_new(4);
    timers->lock = malloc(sizeof(uv_mutex_t));
    timers->stat_fn = NULL;

    if (!timers->timer || !timers->timers || !timers->lock ||
        uv_mutex_init(timers->lock))
        goto failed;

    ti.timers = timers;
    return 0;

failed:
    timers__destroy(NULL);
    ti.timers = NULL;
    return -1;
}

static void timers__reschedule(vec_t * timers)
{
    uint64_t now = util_now_usec();
    uint32_t rel_id = ti.rel_id;
    uint32_t nodes_n = ti.nodes->vec->n;

    for (vec_each(timers, ti_timer_t, timer))
    {
        if (timer->next_run > now)
            continue;

        if (timer->id % nodes_n == rel_id)
        {
            if (timer->repeat)
            {
                uint64_t until = now - timer->repeat;
                while (timer->next_run <= until)
                    timer->next_run += timer->repeat;
            }
            continue;
        }

        if (timer->repeat)
            while (timer->next_run <= now)
                timer->next_run += timer->repeat;
    }
}

static void timers__reschedule_all(void)
{
    timers__reschedule(timers->timers);

    for (vec_each(ti.collections->vec, ti_collection_t, collection))
        timers__reschedule(collection->timers);
}

static int timers__write_to_stat(char * stat_fn)
{
    msgpack_packer pk;
    FILE * f = fopen(stat_fn, "w");
    if (!f)
    {
        log_errno_file("cannot open file", errno, stat_fn);
        return -1;
    }

    uv_mutex_lock(timers->lock);

    msgpack_packer_init(&pk, f, msgpack_fbuffer_write);

    if (msgpack_pack_array(&pk, ti.collections->vec->n + 1) ||
        msgpack_pack_array(&pk, 2) ||
        msgpack_pack_uint64(&pk, TI_SCOPE_THINGSDB) ||
        msgpack_pack_array(&pk, timers->timers->n)
    ) goto fail;

    for (vec_each(timers->timers, ti_timer_t, timer))
    {
        if (msgpack_pack_array(&pk, 2) ||
            msgpack_pack_uint64(&pk, timer->id) ||
            msgpack_pack_uint64(&pk, timer->next_run))
            goto fail;
    }

    for (vec_each(ti.collections->vec, ti_collection_t, collection))
    {
        if (msgpack_pack_array(&pk, 2) ||
            msgpack_pack_uint64(&pk, collection->root->id) ||
            msgpack_pack_array(&pk, collection->timers->n)
        ) goto fail;

        for (vec_each(collection->timers, ti_timer_t, timer))
        {
            if (msgpack_pack_array(&pk, 2) ||
                msgpack_pack_uint64(&pk, timer->id) ||
                msgpack_pack_uint64(&pk, timer->next_run))
                goto fail;
        }
    }

    log_debug("stored timers status to file: `%s`", stat_fn);
    goto done;

fail:
    log_error("failed to write file: `%s`", stat_fn);
done:
    uv_mutex_unlock(timers->lock);

    if (fclose(f))
    {
        log_errno_file("cannot close file", errno, stat_fn);
        return -1;
    }
    return 0;
}

int timers__load_from_stat(char * stat_fn)
{
    int rc = -1;
    size_t i, ii;
    ssize_t n;
    mp_obj_t obj, mp_id, mp_next_run;
    mp_unp_t up;
    uchar * data;
    vec_t * vtimers;
    ti_collection_t * collection;
    ti_timer_t * timer;

    if (!fx_file_exist(stat_fn))
    {
        log_debug("timers status file does not exist: `%s`", stat_fn);
        return 0;
    }

    data = fx_read(stat_fn, &n);
    if (!data)
        goto fail;

    mp_unp_init(&up, data, (size_t) n);

    if (mp_next(&up, &obj) != MP_ARR)
        goto fail;

    for (i = obj.via.sz; i--;)
    {
        if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 2 ||
            mp_next(&up, &mp_id) != MP_U64 ||
            mp_next(&up, &obj) != MP_ARR)
            goto fail;

        collection = mp_id.via.u64 != TI_SCOPE_THINGSDB
                ? ti_collections_get_by_id(mp_id.via.u64)
                : NULL;
        vtimers = collection ? collection->timers : timers->timers;

        for (ii = obj.via.sz; ii--;)
        {
            if (mp_next(&up, &obj) != MP_ARR || obj.via.sz != 2 ||
                mp_next(&up, &mp_id) != MP_U64 ||
                mp_next(&up, &mp_next_run) != MP_U64)
                goto fail;

            timer = ti_timer_by_id(vtimers, mp_id.via.u64);
            if (timer && mp_next_run.via.u64 > timer->next_run)
                timer->next_run = mp_next_run.via.u64;
        }
    }

    rc = 0;
fail:
    if (rc)
        log_error("failed to restore from file: `%s`", stat_fn);

    free(data);
    return rc;
}

int ti_timers_start(void)
{
    assert (timers->is_started == false);

    char * stat_fn = timers__get_fn();
    if (!stat_fn || uv_timer_init(ti.loop, timers->timer))
        goto fail0;

    /* load latest timers next run from status file */
    (void) timers__load_from_stat(stat_fn);

    /* re-schedule timers */
    timers__reschedule_all();

    if (uv_timer_start(
            timers->timer,
            timers__cb,
            TIMERS__INTERVAL,       /* start at TIMERS__INTERVAL */
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
    if (!timers || timers->is_stopping)
        return;

    timers->is_stopping = true;

    if (!timers->is_started)
        timers__destroy(NULL);
    else
    {
        (void) timers__write_to_stat(timers->stat_fn);
        timers->is_started = false;
        uv_timer_stop(timers->timer);
        uv_close((uv_handle_t *) timers->timer, timers__destroy);
    }
}

void ti_timers_clear(vec_t ** timers)
{
    ti_timer_t * timer;
    while ((timer = vec_pop(*timers)))
    {
        ti_timer_mark_del(timer);
        ti_timer_drop(timer);
    }

    vec_shrink(timers);
}

void ti_timers_del_user(ti_user_t * user)
{
    for (vec_each(timers->timers, ti_timer_t, timer))
        if (timer->user == user)
            ti_timer_mark_del(timer);

    for (vec_each(ti.collections->vec, ti_collection_t, collection))
        for (vec_each(collection->timers, ti_timer_t, timer))
            if (timer->user == user)
                ti_timer_mark_del(timer);
}

vec_t ** ti_timers_from_scope_id(uint64_t scope_id)
{
    if (scope_id == TI_SCOPE_THINGSDB)
        return &timers->timers;

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
        uv_mutex_destroy(timers->lock);
        free(timers->lock);
        free(timers->stat_fn);
    }
    free(timers);
    timers = ti.timers = NULL;
}

static int timer__handle(vec_t * vtimers, uint64_t now, ti_timers_cb cb)
{
    int n = 0;
    size_t i = vtimers->n;
    uint32_t rel_id = ti.rel_id;
    uint32_t nodes_n = ti.nodes->vec->n;

    uv_mutex_lock(timers->lock);

    for (vec_each_rev(vtimers, ti_timer_t, timer))
    {
        --i;

        if (!timer->user)
        {
            ti_timer_drop(vec_swap_remove(vtimers, i));
            continue;
        }

        if (timer->next_run > now)
            continue;

        if (timer->id % nodes_n == rel_id)
        {
            n += !cb(timer);
            continue;
        }

        if ((now - timer->next_run) > TIMERS__EXPIRE_TIME)
        {
            if (timer->repeat)
                timer->next_run += timer->repeat;
            else
                ti_timer_drop(vec_swap_remove(vtimers, i));
        }
    }

    uv_mutex_unlock(timers->lock);

    return n;
}

/*
 * Called from the main thread at CONNECT__INTERVAL (every X seconds)
 */
static void timers__cb(uv_timer_t * UNUSED(handle))
{
    uint64_t now = util_now_usec();
    ti_timers_cb cb;
    int n;

    if ((ti.node->status & (
            TI_NODE_STAT_SYNCHRONIZING|
            TI_NODE_STAT_AWAY|
            TI_NODE_STAT_AWAY_SOON|
            TI_NODE_STAT_READY)) == 0)
        return;

    cb = ti.node->status == TI_NODE_STAT_READY ? ti_timer_run : ti_timer_fwd;

    n = timer__handle(timers->timers, now, cb);

    for (vec_each(ti.collections->vec, ti_collection_t, collection))
        n += timer__handle(collection->timers, now, cb);

    if (n)
        log_info("processed %d timer%s", n, n == 1 ? "": "s");
}

ti_varr_t * ti_timers_info(vec_t * timers, _Bool with_full_access)
{
    ti_val_t * mpinfo;
    ti_varr_t * varr = ti_varr_create(timers->n);
    if (!varr)
        return NULL;

    for (vec_each(timers, ti_timer_t, timer))
    {
        if (!timer->user)
            continue;

        mpinfo = ti_timer_as_mpval(timer, with_full_access);
        if (!mpinfo)
            goto failed;

        VEC_push(varr->vec, mpinfo);
    }

    return varr;

failed:
    ti_val_unsafe_drop((ti_val_t *) varr);
    return NULL;
}

