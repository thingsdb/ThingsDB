/*
 * ti/store.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/store.h>
#include <ti/store/access.h>
#include <ti/store/collection.h>
#include <ti/store/collections.h>
#include <ti/store/names.h>
#include <ti/store/status.h>
#include <ti/store/things.h>
#include <ti/things.h>
#include <ti/users.h>
#include <util/fx.h>
#include <util/imap.h>
#include <util/logger.h>

/* path names */
static const char * store__path          = ".store/";
static const char * store__prev_path     = ".prev_/";
static const char * store__tmp_path      = ".tmp__/";
/* file names */
static const char * store__access_fn            = "access.qp";
static const char * store__collections_fn       = "collections.qp";
static const char * store__id_stat_fn           = "idstat.qp";
static const char * store__names_fn             = "names.qp";
static const char * store__users_fn             = "users.qp";

static int store__thing_drop(ti_thing_t * thing, void * UNUSED(arg));
static void store__set_filename(_Bool use_tmp);

static ti_store_t * store;

int ti_store_create(void)
{
    char * storage_path = ti()->cfg->storage_path;
    assert (storage_path);
    store = malloc(sizeof(ti_store_t));
    if (!store)
        goto fail0;

    /* path names */
    store->tmp_path = fx_path_join(storage_path, store__tmp_path);
    if (!store->tmp_path)
        goto fail0;

    store->fn_offset = strlen(store->tmp_path) - strlen(store__tmp_path);

    store->prev_path = fx_path_join(storage_path, store__prev_path);
    store->store_path = fx_path_join(storage_path, store__path);

    /* file names */
    store->access_fn = fx_path_join(store->tmp_path, store__access_fn);
    store->collections_fn = fx_path_join(
            store->tmp_path,
            store__collections_fn);
    store->id_stat_fn = fx_path_join(store->tmp_path, store__id_stat_fn);
    store->names_fn = fx_path_join(store->tmp_path, store__names_fn);
    store->users_fn = fx_path_join(store->tmp_path, store__users_fn);

    if (    !store->prev_path ||
            !store->store_path ||
            !store->access_fn ||
            !store->collections_fn ||
            !store->id_stat_fn ||
            !store->names_fn ||
            !store->users_fn)
        goto fail1;

    ti()->store = store;
    return 0;
fail1:
    ti_store_destroy();
fail0:
    free(store);
    store = NULL;
    return -1;
}

void ti_store_destroy(void)
{
    if (!store)
        return;
    free(store->prev_path);
    free(store->store_path);
    free(store->tmp_path);

    free(store->access_fn);
    free(store->collections_fn);
    free(store->id_stat_fn);
    free(store->names_fn);
    free(store->users_fn);
    free(store);
    store = NULL;
}

int ti_store_store(void)
{
    assert (store);

    /* not need for checking on errors */
    fx_rmdir(store->prev_path);
    mkdir(store->tmp_path, 0700);

    store__set_filename(true);

    if (    ti_store_status_store(store->id_stat_fn) ||
            ti_store_names_store(store->names_fn) ||
            ti_users_store(store->users_fn) ||
            ti_store_access_store(ti()->access, store->access_fn) ||
            ti_store_collections_store(store->collections_fn))
        goto failed;

    for (vec_each(ti()->collections->vec, ti_collection_t, collection))
    {
        int rc;
        ti_store_collection_t * store_collection = ti_store_collection_create(
                store->tmp_path,
                collection);
        if (!store_collection)
            goto failed;

        rc = (  mkdir(store_collection->collection_path, 0700) ||
                ti_store_access_store(
                        collection->access,
                        store_collection->access_fn) ||
                ti_store_things_store(
                        collection->things,
                        store_collection->things_fn) ||
                ti_collection_store(
                        collection,
                        store_collection->collection_fn) ||
                ti_store_things_store_skeleton(
                        collection->things,
                        store_collection->skeleton_fn) ||
                ti_store_things_store_data(
                        collection->things,
                        store_collection->data_fn));

        ti_store_collection_destroy(store_collection);
        if (rc)
            goto failed;
    }

    (void) rename(store->store_path, store->prev_path);
    if (rename(store->tmp_path, store->store_path))
        goto failed;

    (void) fx_rmdir(store->prev_path);

    return 0;

failed:
    log_error("storing ThingsDB has failed");
    (void) fx_rmdir(store->tmp_path);
    return -1;
}

int ti_store_restore(void)
{
    assert (store);

    store__set_filename(false);

    imap_t * namesmap = ti_store_names_restore(store->names_fn);
    int rc = (
            -(!namesmap) ||
            ti_store_status_restore(store->id_stat_fn) ||
            ti_users_restore(store->users_fn) ||
            ti_store_access_restore(&ti()->access, store->access_fn) ||
            ti_store_collections_restore(store->collections_fn));

    if (rc)
        goto stop;

    for (vec_each(ti()->collections->vec, ti_collection_t, collection))
    {
        ti_store_collection_t * store_collection = ti_store_collection_create(
                store->store_path,
                collection);
        rc = (  -(!store_collection) ||
                ti_store_access_restore(
                        &collection->access,
                        store_collection->access_fn) ||
                ti_store_things_restore(
                        collection->things,
                        store_collection->things_fn) ||
                ti_collection_restore(
                        collection,
                        store_collection->collection_fn) ||
                ti_store_things_restore_skeleton(
                        collection->things,
                        namesmap,
                        store_collection->skeleton_fn) ||
                ti_store_things_restore_data(
                        collection->things,
                        namesmap,
                        store_collection->data_fn));

        ti_store_collection_destroy(store_collection);

        assert (collection->root);

        if (rc)
            goto stop;

        (void) imap_walk(
                collection->things,
                (imap_cb) store__thing_drop,
                NULL);
    }

stop:
    if (namesmap)
        imap_destroy(namesmap, NULL);
    return rc;
}

static int store__thing_drop(ti_thing_t * thing, void * UNUSED(arg))
{
    assert (thing->ref > 1);
    --thing->ref;
    return 0;
}

static void store__set_filename(_Bool use_tmp)
{
    const char * path = use_tmp ? store__tmp_path : store__path;
    size_t n = strlen(path);
    memcpy(store->access_fn + store->fn_offset, path, n);
    memcpy(store->collections_fn + store->fn_offset, path, n);
    memcpy(store->id_stat_fn + store->fn_offset, path, n);
    memcpy(store->names_fn + store->fn_offset, path, n);
    memcpy(store->users_fn + store->fn_offset, path, n);
}


