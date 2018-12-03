/*
 * collections.h
 */
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ti/collection.h>
#include <ti/collections.h>
#include <ti/auth.h>
#include <ti/proto.h>
#include <ti/access.h>
#include <ti/things.h>
#include <ti.h>
#include <util/vec.h>

static ti_collections_t * collections = NULL;

int ti_collections_create(void)
{
    collections = malloc(sizeof(ti_collections_t));
    if (!collections)
        goto failed;

    collections->vec = vec_new(1);
    if (!collections->vec)
        goto failed;

    ti()->collections = collections;
    return 0;

failed:
    free(collections);
    return -1;
}

void ti_collections_destroy(void)
{
    if (!collections)
        return;
    vec_destroy(collections->vec, (vec_destroy_cb) ti_collection_drop);
    free(collections);
    ti()->collections = collections = NULL;
}

ti_collection_t * ti_collections_create_collection(
        uint64_t root_id,
        const char * name,
        size_t n,
        ti_user_t * user,
        ex_t * e)
{
    guid_t guid;
    ti_collection_t * collection = NULL;

    if (!ti_name_is_valid_strn(name, n))
    {
        ex_set(e, EX_BAD_DATA,
                "collection name should be a valid name, "
                "see "TI_DOCS"#names");
        goto fail0;
    }

    if (ti_collections_get_by_strn(name, n))
    {
        ex_set(e, EX_INDEX_ERROR,
                "collection `%.*s` already exists", (int) n, name);
        goto fail0;
    }

    if (root_id && ti_collections_get_by_id(root_id))
    {
        ex_set(e, EX_INDEX_ERROR, TI_COLLECTION_ID" already exists", root_id);
        goto fail0;
    }

    if (root_id >= *ti_.next_thing_id)
        ++(*ti_.next_thing_id);
    else
        root_id = ti_next_thing_id();

    guid_init(&guid, root_id);

    collection = ti_collection_create(&guid, name, n);
    if (!collection || vec_push(&collections->vec, collection))
    {
        ex_set_alloc(e);
        goto fail0;
    }

    collection->root = ti_things_create_thing(collection->things, root_id);

    if (!collection->root || ti_access_grant(
            &collection->access,
            user,
            TI_AUTH_MASK_FULL))
    {
        ex_set_alloc(e);
        goto fail1;
    }

    return collection;

fail1:
    (void *) vec_pop(collections->vec);
fail0:
    ti_collection_drop(collection);
    return NULL;
}


ti_collection_t * ti_collections_get_by_raw(const ti_raw_t * raw)
{
    for (vec_each(collections->vec, ti_collection_t, collection))
        if (ti_raw_equal(collection->name, raw))
            return collection;
    return NULL;
}

/* returns a weak reference */
ti_collection_t * ti_collections_get_by_strn(const char * str, size_t n)
{
    for (vec_each(collections->vec, ti_collection_t, collection))
        if (ti_raw_equal_strn(collection->name, str, n))
            return collection;
    return NULL;
}

/* returns a weak reference */
ti_collection_t * ti_collections_get_by_id(const uint64_t id)
{
    for (vec_each(collections->vec, ti_collection_t, collection))
        if (id == collection->root->id)
            return collection;
    return NULL;
}

/*
 * Returns a weak reference collection based on a QPack object.
 * If the collection is not found, then e will contain the reason why.
 * - if the collection target is `root`, then the return value is NULL and
 *   e->nr is EX_SUCCESS
 */
ti_collection_t * ti_collections_get_by_qp_obj(qp_obj_t * obj, ex_t * e)
{
    ti_collection_t * collection = NULL;
    switch (obj->tp)
    {
    case QP_RAW:
        collection = ti_collections_get_by_strn((char *) obj->via.raw, obj->len);
        if (!collection)
            ex_set(
                e,
                EX_INDEX_ERROR,
                "collection `%.*s` not found",
                obj->len,
                (char *) obj->via.raw);
        break;
    case QP_INT64:
        if (!obj->via.int64)
            ex_set(e, EX_SUCCESS, "collection target is root");
        else
        {
            uint64_t id = (uint64_t) obj->via.int64;
            collection = ti_collections_get_by_id(id);
            if (!collection)
                ex_set(
                    e,
                    EX_INDEX_ERROR,
                    TI_COLLECTION_ID" not found",
                    id);
        }
        break;
    default:
        ex_set(e, EX_BAD_DATA, "expecting a `name` or `id` as target");
    }
    return collection;
}

/*
 * Returns a weak reference collection based on a ti_val_t.
 * If the collection is not found, then e will contain the reason why.
 * - if the collection target is `root`, then the return value is NULL and
 *   e->nr is EX_SUCCESS
 */
ti_collection_t * ti_collections_get_by_val(ti_val_t * val, ex_t * e)
{
    ti_collection_t * collection = NULL;
    switch (val->tp)
    {
    case TI_VAL_RAW:
        collection = ti_collections_get_by_strn(
                (const char *) val->via.raw->data, val->via.raw->n);
        if (!collection)
            ex_set(
                e,
                EX_INDEX_ERROR,
                "collection `%.*s` not found",
                val->via.raw->n,
                (char *) val->via.raw->data);
        break;
    case TI_VAL_INT:
        if (val->via.int_ == 0)
            ex_set(e, EX_SUCCESS, "collection target is root");
        else
        {
            uint64_t id = (uint64_t) val->via.int_;
            collection = ti_collections_get_by_id(id);
            if (!collection)
                ex_set(
                    e,
                    EX_INDEX_ERROR,
                    TI_COLLECTION_ID" not found",
                    id);
        }
        break;
    default:
        ex_set(e, EX_BAD_DATA, "expecting type `%s` or `%s` as target",
                ti_val_tp_str(TI_VAL_RAW), ti_val_tp_str(TI_VAL_INT));
    }
    return collection;
}

