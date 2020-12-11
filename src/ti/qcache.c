#include <ti/qcache.h>
#include <tiinc.h>
#include <util/smap.h>
#include <util/util.h>

static smap_t * qcache;

typedef struct
{
    uint32_t ref;
    uint32_t chk_val_n;
    uint64_t last;
    ti_query_t * query;
} qcache__item_t;

static void qcache__item_destroy(qcache__item_t * item)
{
    ti_query_destroy(item->query);
    free(item);
}

static ti_query_t * qcache__from_cache(qcache__item_t * item, uint8_t flags)
{
    ti_query_t * query = malloc(
            flags|TI_QUERY_FLAG_CACHE|TI_QUERY_FLAG_IN_CACHE);
    if (!query)
        return NULL;

    item->last = util_now_tsec();
    query->parseres = item->query->parseres;
    query->querystr = item->query->querystr;
    query->qbind = item->query->qbind;
    query->val_cache = vec_copy(item->query->val_cache);
    if (!query->val_cache)
    {
        query_destroy(query);
        return NULL;
    }
    return query;
}

int ti_qcache_create(void)
{
    if (!ti.cfg->threshold_query_cache)
        ti.cfg->threshold_query_cache = SIZE_MAX;
    qcache = smap_create();
    ti.qcache = qcache;
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
    if (n < ti.cfg->threshold_query_cache)
        return ti_create_query(flags);
    item = smap_getn(qcache, str, n);
    return item
            ? qcache__from_cache(item, flags)
            : ti_create_query(flags|TI_QUERY_FLAG_CACHE);
}
