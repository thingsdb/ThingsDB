/*
 * ti/store/collection.c
 */
#include <assert.h>
#include <ti.h>
#include <util/fx.h>
#include <stdlib.h>
#include <ti/store/storecollection.h>

static const char * collection___access_fn     = "access.mp";
static const char * collection___dat_fn        = "collection.dat";
static const char * collection___procedures_fn = "procedures.mp";
static const char * collection___props_fn      = "props.mp";
static const char * collection___things_fn     = "things.mp";
static const char * collection___types_fn      = "types.mp";
static const char * collection___enums_fn      = "enums.mp";

ti_store_collection_t * ti_store_collection_create(
        const char * path,
        guid_t * guid)
{
    char * cpath;
    ti_store_collection_t * store_collection;

    store_collection = malloc(sizeof(ti_store_collection_t));
    if (!store_collection)
        goto fail0;

    cpath = store_collection->collection_path = fx_path_join(
            path,
            guid->guid);

    if (!cpath)
        goto fail0;

    store_collection->access_fn = fx_path_join(cpath, collection___access_fn);
    store_collection->collection_fn = fx_path_join(cpath, collection___dat_fn);
    store_collection->procedures_fn = fx_path_join(cpath, collection___procedures_fn);
    store_collection->props_fn = fx_path_join(cpath, collection___props_fn);
    store_collection->things_fn = fx_path_join(cpath, collection___things_fn);
    store_collection->types_fn = fx_path_join(cpath, collection___types_fn);
    store_collection->enums_fn = fx_path_join(cpath, collection___enums_fn);

    if (    !store_collection->access_fn ||
            !store_collection->collection_fn ||
            !store_collection->procedures_fn ||
            !store_collection->props_fn ||
            !store_collection->things_fn ||
            !store_collection->types_fn ||
            !store_collection->enums_fn)
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
    free(store_collection->collection_fn);
    free(store_collection->collection_path);
    free(store_collection->procedures_fn);
    free(store_collection->props_fn);
    free(store_collection->things_fn);
    free(store_collection->types_fn);
    free(store_collection->enums_fn);
    free(store_collection);
}

int ti_store_collection_store(ti_collection_t * collection, const char * fn)
{
    int rc = 0;
    FILE * f = fopen(fn, "w");
    if (!f)
    {
        log_errno_file("cannot open file", errno, fn);
        return -1;
    }

    if (fwrite(&collection->root->id, sizeof(uint64_t), 1, f) != 1)
    {
        log_error("cannot write to file `%s`", fn);
        rc = -1;
    }

    if (fclose(f))
    {
        log_errno_file("cannot close file", errno, fn);
        rc = -1;
    }

    if (rc == 0)
        log_debug("stored collection info to file: `%s`", fn);

    return rc;
}

int ti_store_collection_restore(ti_collection_t * collection, const char * fn)
{
    int rc = 0;
    ssize_t sz;
    uint64_t id;
    uchar * data = fx_read(fn, &sz);
    if (!data || sz != sizeof(uint64_t))
        goto failed;

    memcpy(&id, data, sizeof(uint64_t));

    collection->root = imap_get(collection->things, id);
    if (!collection->root)
    {
        log_critical("cannot find collection root: "TI_THING_ID, id);
        goto failed;
    }

    ti_incref(collection->root);
    goto done;

failed:
    rc = -1;
    log_critical("failed to restore from file: `%s`", fn);

done:
    free(data);
    return rc;
}

_Bool ti_store_collection_is_stored(const char * path, uint64_t collection_id)
{
    _Bool exists;

    char * collection_path = ti_store_collection_get_path(path, collection_id);
    exists = fx_is_dir(collection_path);

    free(collection_path);
    return exists;
}

char * ti_store_collection_get_path(const char * path, uint64_t collection_id)
{
    char * collection_path;
    guid_t guid;
    guid_init(&guid, collection_id);
    collection_path = fx_path_join(path, guid.guid);
    return collection_path;
}

char * ti_store_collection_access_fn(
        const char * path,
        uint64_t collection_id)
{
    char * fn, * cpath = ti_store_collection_get_path(path, collection_id);
    if (!cpath)
        return NULL;
    fn = fx_path_join(cpath, collection___access_fn);
    free(cpath);
    return fn;
}

char * ti_store_collection_procedures_fn(
        const char * path,
        uint64_t collection_id)
{
    char * fn, * cpath = ti_store_collection_get_path(path, collection_id);
    if (!cpath)
        return NULL;
    fn = fx_path_join(cpath, collection___procedures_fn);
    free(cpath);
    return fn;
}

char * ti_store_collection_dat_fn(
        const char * path,
        uint64_t collection_id)
{
    char * fn, * cpath = ti_store_collection_get_path(path, collection_id);
    if (!cpath)
        return NULL;
    fn = fx_path_join(cpath, collection___dat_fn);
    free(cpath);
    return fn;
}

char * ti_store_collection_props_fn(
        const char * path,
        uint64_t collection_id)
{
    char * fn, * cpath = ti_store_collection_get_path(path, collection_id);
    if (!cpath)
        return NULL;
    fn = fx_path_join(cpath, collection___props_fn);
    free(cpath);
    return fn;
}

char * ti_store_collection_things_fn(
        const char * path,
        uint64_t collection_id)
{
    char * fn, * cpath = ti_store_collection_get_path(path, collection_id);
    if (!cpath)
        return NULL;
    fn = fx_path_join(cpath, collection___things_fn);
    free(cpath);
    return fn;
}

char * ti_store_collection_types_fn(
        const char * path,
        uint64_t collection_id)
{
    char * fn, * cpath = ti_store_collection_get_path(path, collection_id);
    if (!cpath)
        return NULL;
    fn = fx_path_join(cpath, collection___types_fn);
    free(cpath);
    return fn;
}

char * ti_store_collection_enums_fn(
        const char * path,
        uint64_t collection_id)
{
    char * fn, * cpath = ti_store_collection_get_path(path, collection_id);
    if (!cpath)
        return NULL;
    fn = fx_path_join(cpath, collection___enums_fn);
    free(cpath);
    return fn;
}
