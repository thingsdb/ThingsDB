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

        if (!collection->things->n)
            imap_destroy(collection->things, NULL);
        else if (ti_collections_add_for_collect(collection->things))
            log_critical(EX_ALLOC_S);

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

ti_val_t * ti_collection_as_qpval(ti_collection_t * collection)
{
    ti_raw_t * raw;
    ti_val_t * qpval = NULL;
    qp_packer_t * packer = qp_packer_create2(128, 1);
    if (!packer)
        return NULL;

    if (ti_collection_to_packer(collection, &packer))
        goto fail;

    raw = ti_raw_from_packer(packer);
    if (!raw)
        goto fail;

    qpval = ti_val_weak_create(TI_VAL_QP, raw);
    if (!qpval)
        ti_raw_drop(raw);

fail:
    qp_packer_destroy(packer);
    return qpval;
}

void ti_collection_set_quota(
        ti_collection_t * collection,
        ti_quota_enum_t quota_tp,
        size_t quota)
{
    switch (quota_tp)
    {
    case QUOTA_THINGS:
        collection->quota->max_things = quota;
        break;
    case QUOTA_PROPS:
        collection->quota->max_props = quota;
        break;
    case QUOTA_ARRAY_SIZE:
        collection->quota->max_array_size = quota;
        break;
    case QUOTA_RAW_SIZE:
        collection->quota->max_raw_size = quota;
        break;
    }
}
