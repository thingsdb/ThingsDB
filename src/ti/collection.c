/*
 * ti/collection.c
 */
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ti/collection.h>
#include <ti/things.h>
#include <ti/name.h>
#include <ti/auth.h>
#include <ti/thing.h>
#include <ti/name.h>
#include <ti/names.h>
#include <ti.h>
#include <util/strx.h>
#include <util/fx.h>

static const size_t ti_collection_min_name = 1;
static const size_t ti_collection_max_name = 128;

ti_collection_t * ti_collection_create(
        guid_t * guid,
        const char * name,
        size_t n)
{
    ti_collection_t * collection = malloc(sizeof(ti_collection_t));
    if (!collection)
        return NULL;

    collection->ref = 1;
    collection->root = NULL;
    collection->name = ti_raw_create((uchar *) name, n);
    collection->things = imap_create();
    collection->access = vec_new(1);
    collection->quota = ti_quota_create();
    collection->lock = malloc(sizeof(uv_mutex_t));


    memcpy(&collection->guid, guid, sizeof(guid_t));

    if (!collection->name || !collection->things || !collection->access ||
        !collection->quota || !collection->lock ||
        uv_mutex_init(collection->lock))
    {
        ti_collection_drop(collection);
        return NULL;
    }

    return collection;
}

void ti_collection_drop(ti_collection_t * collection)
{
    if (collection && !--collection->ref)
    {
        ti_raw_drop(collection->name);
        vec_destroy(collection->access, (vec_destroy_cb) ti_auth_destroy);

        ti_thing_drop(collection->root);
        ti_things_gc(collection->things, NULL);

        assert (collection->things->n == 0);
        imap_destroy(collection->things, NULL);
        ti_quota_destroy(collection->quota);
        uv_mutex_destroy(collection->lock);
        free(collection->lock);
        free(collection);
    }
}

_Bool ti_collection_name_check(const char * name, size_t n, ex_t * e)
{
    if (n < ti_collection_min_name || n >= ti_collection_max_name)
    {
        ex_set(e, EX_BAD_DATA,
                "collection name must be between %u and %u characters",
                ti_collection_min_name,
                ti_collection_max_name);
        return false;
    }

    if (!ti_name_is_valid_strn(name, n))
    {
        ex_set(e, EX_BAD_DATA,
                "collection name should be a valid name, "
                "see "TI_DOCS"#names");
        return false;
    }

    return true;
}


