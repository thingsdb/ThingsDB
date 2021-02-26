/*
 * ti/collections.c
 */
#include <assert.h>
#include <doc.h>
#include <stdlib.h>
#include <string.h>
#include <ti.h>
#include <ti/access.h>
#include <ti/auth.h>
#include <ti/collection.h>
#include <ti/collection.inline.h>
#include <ti/collections.h>
#include <ti/enums.h>
#include <ti/proto.h>
#include <ti/things.h>
#include <ti/val.inline.h>
#include <ti/vint.h>
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

    ti.collections = collections;
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

    ti.collections = collections = NULL;
}

void ti_collections_clear(void)
{
    while (collections->vec->n)
        ti_collection_drop(VEC_pop(collections->vec));
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
        if (ti_collection_gc(collection, true))
        {
            log_error("garbage collection for collection `%.*s` has failed",
                    collection->name->n, (char *) collection->name->data);
            rc = -1;
        }

        ti_sleep(100);
    }

    return rc;
}

_Bool ti_collections_del_collection(const uint64_t collection_id)
{
    uint32_t i = 0;
    for (vec_each(collections->vec, ti_collection_t, collection), ++i)
    {
        if (collection->root->id == collection_id)
        {
            ti_collection_drop(vec_swap_remove(collections->vec, i));
            return true;
        }
    }
    return false;
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
        /* TODO: do something with timers. */

        /* stop exiting futures */
        ti_collection_stop_futures(collection);

        /* drop enumerators; this is required since we no longer mark the
         * enumerators when they contain things so they need to be dropped
         * before the garbage collector will remove them */
        ti_enums_destroy(collection->enums);
        collection->enums = NULL;

        if (ti_collection_gc(collection, false))
        {
            rc = -1;
            log_critical(EX_MEMORY_S);
            continue;
        }

        ti_collection_gc_clear(collection);
        ti_collection_destroy(collection);
    }
    return rc;
}

ti_collection_t * ti_collections_create_collection(
        uint64_t root_id,
        const char * name,
        size_t name_n,
        uint64_t created_at,
        ti_user_t * user,
        ex_t * e)
{
    guid_t guid;
    ti_collection_t * collection = NULL;
    ti_tz_t * tz = ti_tz_utc();

    if (!ti_name_is_valid_strn(name, name_n))
    {
        ex_set(e, EX_VALUE_ERROR,
                "collection name must follow the naming rules"
                DOC_NAMES);
        goto fail0;
    }

    if (ti_collections_get_by_strn(name, name_n))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "collection `%.*s` already exists", name_n, name);
        goto fail0;
    }

    if (root_id && ti_collections_get_by_id(root_id))
    {
        ex_set(e, EX_LOOKUP_ERROR, TI_COLLECTION_ID" already exists", root_id);
        goto fail0;
    }

    if (root_id >= ti.node->next_thing_id)
        ++ti.node->next_thing_id;
    else
        root_id = ti_next_thing_id();

    guid_init(&guid, root_id);

    collection = ti_collection_create(&guid, name, name_n, tz, created_at);
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
            uint64_t id = (uint64_t) VINT(val);
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

/*
 * If `user` is NULL, then all collection info will be returned. When a `user`
 * is given, only collection info where the user has at least `QUERY` or `RUN`
 * permissions are returned.
 */
ti_varr_t * ti_collections_info(ti_user_t * user)
{
    vec_t * vec = collections->vec;
    ti_varr_t * varr = ti_varr_create(vec->n);
    if (!varr)
        return NULL;

    for (vec_each(vec, ti_collection_t, collection))
    {
        ti_val_t * mpinfo;

        if (user && !ti_access_check_or(
                collection->access,
                user,
                TI_AUTH_QUERY|TI_AUTH_RUN))
            continue;

        mpinfo = ti_collection_as_mpval(collection);
        if (!mpinfo)
        {
            ti_val_unsafe_drop((ti_val_t *) varr);
            return NULL;
        }
        VEC_push(varr->vec, mpinfo);
    }
    return varr;
}
