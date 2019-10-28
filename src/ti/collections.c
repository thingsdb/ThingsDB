/*
 * ti/collections.c
 */
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ti/collection.h>
#include <ti/collection.inline.h>
#include <ti/collections.h>
#include <ti/auth.h>
#include <ti/proto.h>
#include <ti/vint.h>
#include <ti/access.h>
#include <ti/things.h>
#include <ti.h>
#include <util/vec.h>

static ti_collections_t * collections;
static ti_collections_t collections_;

int ti_collections_create(void)
{
    collections = &collections_;

    collections->vec = vec_new(1);
    collections->dropped = vec_new(1);
    if (!collections->vec || !collections->dropped)
        goto failed;

    ti()->collections = collections;
    return 0;

failed:
    vec_destroy(collections->vec, NULL);
    vec_destroy(collections->dropped, NULL);
    free(collections);
    collections = NULL;
    return -1;
}

void ti_collections_destroy(void)
{
    if (!collections)
        return;
    vec_destroy(collections->vec, (vec_destroy_cb) ti_collection_drop);

    (void) ti_collections_gc_collect_dropped();
    assert (collections->dropped->n == 0);
    vec_destroy(collections->dropped, NULL);

    ti()->collections = collections = NULL;
}

void ti_collections_clear(void)
{
    while (collections->vec->n)
        ti_collection_drop(vec_pop(collections->vec));
}

/*
 * Returns 0 when successful, -1 indicates one or more actions have failed and
 * logging is done.
 */
int ti_collections_gc(void)
{
    /* garbage collect dropped collections */
    int rc = ti_collections_gc_collect_dropped();

    /* collect all other stuff */
    for (vec_each(collections->vec, ti_collection_t, collection))
    {
        uv_mutex_lock(collection->lock);

        if (ti_things_gc(collection->things, collection->root))
        {
            log_error("garbage collection for collection `%.*s` has failed",
                    (int) collection->name->n,
                    (char *) collection->name->data);
            rc = -1;
        }

        uv_mutex_unlock(collection->lock);

        ti_sleep(100);
    }

    return rc;
}

void ti_collections_cleanup(void)
{
    for (vec_each(collections->vec, ti_collection_t, collection))
        imap_vec_clear(collection->things);
}

_Bool ti_collections_del_collection(const uint64_t collection_id)
{
    uint32_t i = 0;
    for (vec_each(collections->vec, ti_collection_t, collection), ++i)
        if (collection->root->id == collection_id)
            break;
    if (i == collections->vec->n)
        return false;
    ti_collection_drop(vec_swap_remove(collections->vec, i));
    return true;
}

int ti_collections_add_for_collect(ti_collection_t * collection)
{
    return vec_push(&collections->dropped, collection);
}

int ti_collections_gc_collect_dropped(void)
{
    int rc = 0;
    ti_collection_t * collection;
    while ((collection = vec_pop(collections->dropped)))
    {
        if (ti_things_gc(collection->things, NULL))
        {
            rc = -1;
            log_critical(EX_MEMORY_S);
            continue;
        }

        assert (!collection->things->n);
        ti_collection_destroy(collection);
    }
    return rc;
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
        ex_set(e, EX_VALUE_ERROR,
                "collection name must follow the naming rules"
                DOC_NAMES);
        goto fail0;
    }

    if (ti_collections_get_by_strn(name, n))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "collection `%.*s` already exists", (int) n, name);
        goto fail0;
    }

    if (root_id && ti_collections_get_by_id(root_id))
    {
        ex_set(e, EX_LOOKUP_ERROR, TI_COLLECTION_ID" already exists", root_id);
        goto fail0;
    }

    if (root_id >= ti_.node->next_thing_id)
        ++ti_.node->next_thing_id;
    else
        root_id = ti_next_thing_id();

    guid_init(&guid, root_id);

    collection = ti_collection_create(&guid, name, n);
    if (!collection || vec_push(&collections->vec, collection))
    {
        ex_set_mem(e);
        goto fail0;
    }

    collection->root = ti_things_create_thing_o(root_id, 8, collection);

    if (!collection->root || ti_access_grant(
            &collection->access,
            user,
            TI_AUTH_MASK_FULL))
    {
        ex_set_mem(e);
        goto fail1;
    }

    return collection;

fail1:
    (void) vec_pop(collections->vec);
fail0:
    ti_collection_drop(collection);
    return NULL;
}

/* returns a weak reference */
ti_collection_t * ti_collections_get_by_strn(const char * str, size_t n)
{
    for (vec_each(collections->vec, ti_collection_t, collection))
        if (ti_raw_eq_strn(collection->name, str, n))
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
 * Returns a weak reference collection based on a ti_val_t.
 * If the collection is not found, then `e` will contain the reason why.
 */
ti_collection_t * ti_collections_get_by_val(ti_val_t * val, ex_t * e)
{
    ti_collection_t * collection = NULL;
    switch (val->tp)
    {
    case TI_VAL_NAME:
    case TI_VAL_STR:
        collection = ti_collections_get_by_strn(
                (const char *) ((ti_raw_t *) val)->data,
                ((ti_raw_t *) val)->n);
        if (!collection)
            ex_set(
                e,
                EX_LOOKUP_ERROR,
                "collection `%.*s` not found",
                ((ti_raw_t *) val)->n,
                (char *) ((ti_raw_t *) val)->data);
        break;
    case TI_VAL_INT:
        {
            uint64_t id = (uint64_t) ((ti_vint_t *) val)->int_;
            collection = ti_collections_get_by_id(id);
            if (!collection)
                ex_set(
                    e,
                    EX_LOOKUP_ERROR,
                    TI_COLLECTION_ID" not found",
                    id);
        }
        break;
    default:
        ex_set(e, EX_TYPE_ERROR,
                "expecting type `"TI_VAL_STR_S"` "
                "or `"TI_VAL_INT_S"` as collection but got type `%s` instead",
                ti_val_str(val));
    }
    return collection;
}

ti_varr_t * ti_collections_info(void)
{
    vec_t * vec = collections->vec;
    ti_varr_t * varr = ti_varr_create(vec->n);
    if (!varr)
        return NULL;

    for (vec_each(vec, ti_collection_t, collection))
    {
        ti_val_t * mpinfo = ti_collection_as_mpval(collection);
        if (!mpinfo)
        {
            ti_val_drop((ti_val_t *) varr);
            return NULL;
        }
        VEC_push(varr->vec, mpinfo);
    }
    return varr;
}
