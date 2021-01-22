/*
 * ti/qcache.c
 */
#include <ti/api.h>
#include <ti/event.h>
#include <ti/prop.h>
#include <ti/qcache.h>
#include <ti/query.h>
#include <ti/thing.h>
#include <ti/val.h>
#include <ti/val.inline.h>
#include <ti.h>
#include <tiinc.h>
#include <util/smap.h>
#include <util/util.h>
#include <util/logger.h>

static smap_t * qcache;
static size_t qcache__cache_sz;
static size_t qcache__threshold;

/* If more than X queries are stored in cache, the cache will ask for away mode
 * to perform a cleanup. */
#define QCACHE__GROW_SIZE 20

typedef struct
{
    uint32_t used;
    uint32_t last;      /* works fine until we reach year 2038 */
    ti_query_t * query;
} qcache__item_t;

static void qcache__item_destroy(qcache__item_t * item)
{
    if (item)
        ti_query_destroy(item->query);
    free(item);
}

static ti_query_t * qcache__from_cache(qcache__item_t * item, uint8_t flags)
{
    ti_query_t * query = ti_query_create(flags|TI_QUERY_FLAG_CACHE);
    if (!query)
        return NULL;

    item->used++;
    item->last = (uint32_t) util_now_tsec();
    /*
     * Mark the cached query so we know this query is at least once being
     * asked from cache.
     */
    item->query->flags |= TI_QUERY_FLAG_CACHE;

    query->with.parseres = item->query->with.parseres;
    query->qbind = item->query->qbind;
    query->immutable_cache = item->query->immutable_cache;

    ++ti.counters->queries_from_cache;

    return query;
}

int ti_qcache_create(void)
{
    qcache__threshold = ti.cfg->cache_expiration_time
            ? ti.cfg->threshold_query_cache
            : SIZE_MAX;  /* this will effectively disable caching */
    qcache = smap_create();
    ti.qcache = qcache;
    qcache__cache_sz = QCACHE__GROW_SIZE;
    return -(!qcache);
}

void ti_qcache_destroy(void)
{
    if (!qcache)
        return;
    smap_destroy(qcache, (smap_destroy_cb) qcache__item_destroy);
    ti.qcache = NULL;
}

ti_query_t * ti_qcache_get_query(const char * str, size_t n, uint8_t flags)
{
    qcache__item_t * item;

    if (n < qcache__threshold)
        return ti_query_create(flags);

    item = smap_getn(qcache, str, n);
    if (item)
        return qcache__from_cache(item, flags);

    flags |= TI_QUERY_FLAG_CACHE|TI_QUERY_FLAG_DO_CACHE;
    return ti_query_create(flags);
}

void ti_qcache_return(ti_query_t * query)
{
    assert (query->with_tp == TI_QUERY_WITH_PARSERES);
    if (query->flags & TI_QUERY_FLAG_API)
        ti_api_release(query->via.api_request);
    else
        ti_stream_drop(query->via.stream);

    ti_user_drop(query->user);
    ti_event_drop(query->ev);
    ti_val_drop(query->rval);

    assert (query->futures.n == 0);

    while(query->vars->n)
        ti_prop_destroy(VEC_pop(query->vars));
    free(query->vars);

    ti_collection_drop(query->collection);

    /*
     * Garbage collection at least after cleaning the return value,
     * otherwise the value might already be destroyed.
     */
    ti_thing_clean_gc();

    if (query->flags & TI_QUERY_FLAG_DO_CACHE)
    {
        qcache__item_t * item = malloc(sizeof(qcache__item_t));
        if (item)
        {
            /* Set the flags to 0, only `TI_QUERY_FLAG_CACHE` will be set
             * on this query if it will be asked at least once from the cache.
             */
            query->flags = 0;
            query->via.stream = NULL;
            query->user = NULL;
            query->ev = NULL;
            query->rval = NULL;
            query->vars = NULL;
            query->collection = NULL;
            item->query = query;
            item->used = 0;
            item->last = (uint32_t) util_now_tsec();
        }
        if (smap_add(qcache, query->with.parseres->str, item))
            qcache__item_destroy(item);
        return;
    }

    free(query);
}

typedef struct
{
    vec_t * items;
    uint32_t expire_ts;
} qcache__cleanup_t;

static int qcache__cleanup_cb(qcache__item_t * item, qcache__cleanup_t * w)
{
    if (item->last < w->expire_ts)
        (void) vec_push(&w->items, item);
    return 0;
}

static void qcache__cleanup_destroy(qcache__item_t * item)
{
    if (!item->used)
        ti_counters_inc_wasted_cache();
    qcache__item_destroy(smap_pop(qcache, item->query->with.parseres->str));
}

void ti_qcache_cleanup(void)
{
    struct timespec now;
    if (clock_gettime(CLOCK_REALTIME, &now))
        return;

    qcache__cleanup_t w = {
        .expire_ts = (uint32_t) now.tv_sec - ti.cfg->cache_expiration_time,
        .items = vec_new(16),
    };

    if (!w.items)
        return;

    (void) smap_values(qcache, (smap_val_cb) qcache__cleanup_cb, &w);

    ti_sleep(100);

    log_info("removed %u item(s) from query cache", w.items->n);

    vec_destroy(w.items, (vec_destroy_cb) qcache__cleanup_destroy);

    qcache__cache_sz = qcache->n + QCACHE__GROW_SIZE;
}

_Bool ti_qcache_require_away(void)
{
    return qcache->n > qcache__cache_sz;
}
