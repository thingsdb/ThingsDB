/*
 * ti/store/collection.c
 */

#include <ti/store/collection.h>
#include <util/fx.h>
#include <stdlib.h>

static const char * ti__store_collection_things_fn     = "things.dat";
static const char * ti__store_collection_collection_fn         = "collection.dat";
static const char * ti__store_collection_skeleton_fn   = "skeleton.qp";
static const char * ti__store_collection_data_fn       = "data.qp";
static const char * ti__store_collection_access_fn     = "access.qp";

ti_store_collection_t * ti_store_collection_create(const char * path, ti_collection_t * collection)
{
    char * collection_path;
    ti_store_collection_t * store_collection = malloc(sizeof(ti_store_collection_t));
    if (!store_collection)
        goto fail0;

    collection_path = store_collection->collection_path = fx_path_join(path, collection->guid.guid);
    if (!collection_path)
        goto fail0;

    store_collection->access_fn = fx_path_join(collection_path, ti__store_collection_access_fn);
    store_collection->things_fn = fx_path_join(collection_path, ti__store_collection_things_fn);
    store_collection->collection_fn = fx_path_join(collection_path, ti__store_collection_collection_fn);
    store_collection->skeleton_fn = fx_path_join(collection_path, ti__store_collection_skeleton_fn);
    store_collection->data_fn = fx_path_join(collection_path, ti__store_collection_data_fn);

    if (    !store_collection->access_fn ||
            !store_collection->things_fn ||
            !store_collection->collection_fn ||
            !store_collection->skeleton_fn ||
            !store_collection->data_fn)
        goto fail1;

    return store_collection;
fail1:
    ti_store_collection_destroy(store_collection);
    return NULL;
fail0:
    free(store_collection);
    return NULL;
}


void ti_store_collection_destroy(ti_store_collection_t * store_collection)
{
    if (!store_collection)
        return;
    free(store_collection->access_fn);
    free(store_collection->things_fn);
    free(store_collection->collection_fn);
    free(store_collection->skeleton_fn);
    free(store_collection->data_fn);
    free(store_collection->collection_path);
    free(store_collection);
}
