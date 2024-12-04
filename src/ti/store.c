/*
 * ti/store.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/name.h>
#include <ti/store.h>
#include <ti/store/storeaccess.h>
#include <ti/store/storecollection.h>
#include <ti/store/storecollections.h>
#include <ti/store/storeenums.h>
#include <ti/store/storegcollect.h>
#include <ti/store/storemodules.h>
#include <ti/store/storenamedrooms.h>
#include <ti/store/storenames.h>
#include <ti/store/storeprocedures.h>
#include <ti/store/storestatus.h>
#include <ti/store/storetasks.h>
#include <ti/store/storethings.h>
#include <ti/store/storetypes.h>
#include <ti/store/storeusers.h>
#include <ti/things.h>
#include <util/fx.h>
#include <util/imap.h>
#include <util/logger.h>

/* path names */
static const char * store__path          = "store/";
static const char * store__prev_path     = "_prev/";
static const char * store__tmp_path      = "__tmp/";

/* file names */
static const char * store__access_node_fn       = "access_node.mp";
static const char * store__access_thingsdb_fn   = "access_thingsdb.mp";
static const char * store__collections_fn       = "collections.mp";
static const char * store__id_stat_fn           = "idstat.mp";
static const char * store__names_fn             = "names.mp";
static const char * store__procedures_fn        = "procedures.mp";
static const char * store__tasks_fn             = "tasks.mp";
static const char * store__users_fn             = "users.mp";
static const char * store__modules_fn           = "modules.mp";

static ti_store_t * store;
static ti_store_t store_;

static int store__thing_drop(ti_thing_t * thing, void * UNUSED(arg))
{
    assert(thing->ref > 1);
    --thing->ref;
    return 0;
}

static void store__set_filename(_Bool use_tmp)
{
    const char * path = use_tmp ? store__tmp_path : store__path;
    size_t n = strlen(path);
    memcpy(store->access_node_fn + store->fn_offset, path, n);
    memcpy(store->access_thingsdb_fn + store->fn_offset, path, n);
    memcpy(store->collections_fn + store->fn_offset, path, n);
    memcpy(store->id_stat_fn + store->fn_offset, path, n);
    memcpy(store->names_fn + store->fn_offset, path, n);
    memcpy(store->procedures_fn + store->fn_offset, path, n);
    memcpy(store->tasks_fn + store->fn_offset, path, n);
    memcpy(store->users_fn + store->fn_offset, path, n);
    memcpy(store->modules_fn + store->fn_offset, path, n);
}

static int store__collection_ids(void)
{
    vec_t * collections_vec = ti.collections->vec;

    vec_destroy(store->collection_ids, free);
    store->collection_ids = vec_new(collections_vec->n);
    if (!store->collection_ids)
        return -1;

    for (vec_each(collections_vec, ti_collection_t, collection))
    {
        uint64_t * id = malloc(sizeof(uint64_t));
        if (!id)
            return -1;
        *id = collection->id;
        VEC_push(store->collection_ids, id);
    }
    return 0;
}

int ti_store_create(void)
{
    char * storage_path = ti.cfg->storage_path;
    assert(storage_path);
    store = &store_;

    /* path names */
    store->tmp_path = fx_path_join(storage_path, store__tmp_path);
    if (!store->tmp_path)
        goto fail0;

    store->fn_offset = strlen(store->tmp_path) - strlen(store__tmp_path);

    store->prev_path = fx_path_join(storage_path, store__prev_path);
    store->store_path = fx_path_join(storage_path, store__path);

    /* file names */
    store->access_node_fn = fx_path_join(
            store->tmp_path,
            store__access_node_fn);
    store->access_thingsdb_fn = fx_path_join(
            store->tmp_path,
            store__access_thingsdb_fn);
    store->collections_fn = fx_path_join(
            store->tmp_path,
            store__collections_fn);
    store->id_stat_fn = fx_path_join(store->tmp_path, store__id_stat_fn);
    store->names_fn = fx_path_join(store->tmp_path, store__names_fn);
    store->procedures_fn = fx_path_join(store->tmp_path, store__procedures_fn);
    store->tasks_fn = fx_path_join(store->tmp_path, store__tasks_fn);
    store->users_fn = fx_path_join(store->tmp_path, store__users_fn);
    store->modules_fn = fx_path_join(store->tmp_path, store__modules_fn);
    store->last_stored_change_id = 0;
    store->collection_ids = NULL;

    if (    !store->prev_path ||
            !store->store_path ||
            !store->access_node_fn ||
            !store->access_thingsdb_fn ||
            !store->collections_fn ||
            !store->id_stat_fn ||
            !store->names_fn ||
            !store->procedures_fn ||
            !store->tasks_fn ||
            !store->users_fn ||
            !store->modules_fn)
        goto fail1;

    ti.store = store;

    store__set_filename(/* use_tmp: */ false);

    return 0;
fail1:
    ti_store_destroy();
fail0:
    store = NULL;
    return -1;
}

int ti_store_init(void)
{
    char * path = ti.store->store_path;

    if (!fx_is_dir(path) && mkdir(path, FX_DEFAULT_DIR_ACCESS))
    {
        log_errno_file("cannot create directory", errno, path);
        return -1;
    }

    return 0;
}

void ti_store_destroy(void)
{
    if (!store)
        return;
    free(store->prev_path);
    free(store->store_path);
    free(store->tmp_path);

    free(store->access_node_fn);
    free(store->access_thingsdb_fn);
    free(store->collections_fn);
    free(store->id_stat_fn);
    free(store->names_fn);
    free(store->procedures_fn);
    free(store->tasks_fn);
    free(store->users_fn);
    free(store->modules_fn);
    vec_destroy(store->collection_ids, free);
    ti.store = store = NULL;
}

/*
 * Make sure the `gc` has ran before calling `ti_store_store`. Otherwise
 * some things may be saved without a reference to a collection.
 */
int ti_store_store(void)
{
    int rc = 0;
    assert(store);

    /* not need for checking on errors */
    (void) fx_rmdir(store->prev_path);
    if (mkdir(store->tmp_path, FX_DEFAULT_DIR_ACCESS))
    {
        log_errno_file("cannot create directory", errno, store->tmp_path);
    }

    (void) ti_sleep(5);

    store__set_filename(/* use_tmp: */ true);

    if (    ti_store_status_store(store->id_stat_fn) ||
            ti_store_names_store(store->names_fn) ||
            ti_store_users_store(store->users_fn) ||
            ti_store_access_store(
                    ti.access_node,
                    store->access_node_fn) ||
            ti_store_access_store(
                    ti.access_thingsdb,
                    store->access_thingsdb_fn) ||
            ti_store_collections_store(store->collections_fn) ||
            ti_store_procedures_store(ti.procedures, store->procedures_fn) ||
            ti_store_tasks_store(ti.tasks->vtasks, store->tasks_fn) ||
            ti_store_modules_store(store->modules_fn))
        goto failed;

    for (vec_each(ti.collections->vec, ti_collection_t, collection))
    {
        ti_store_collection_t * store_collection = ti_store_collection_create(
                store->tmp_path,
                &collection->guid);

        if (!store_collection)
            goto failed;

        (void) ti_sleep(2);

        rc = mkdir(store_collection->collection_path, FX_DEFAULT_DIR_ACCESS);
        if (rc)
        {
            log_errno_file("cannot create collection path",
                    errno, store_collection->collection_path);
        }
        else
        {
            rc = (
                ti_store_enums_store(
                        collection->enums,
                        store_collection->enums_fn) ||
                ti_store_types_store(
                        collection->types,
                        store_collection->types_fn) ||
                ti_store_access_store(
                        collection->access,
                        store_collection->access_fn) ||
                ti_store_things_store(
                        collection->things,
                        store_collection->things_fn) ||
                ti_store_collection_store(
                        collection,
                        store_collection->collection_fn) ||
                ti_store_things_store_data(
                        collection->things,
                        store_collection->props_fn) ||
                ti_store_gcollect_store(
                        collection->gc,
                        store_collection->gcthings_fn) ||
                ti_store_gcollect_store_data(
                        collection->gc,
                        store_collection->gcprops_fn) ||
                ti_store_named_rooms_store(
                        collection->named_rooms,
                        store_collection->named_rooms_fn) ||
                ti_store_procedures_store(
                        collection->procedures,
                        store_collection->procedures_fn) ||
                ti_store_tasks_store(
                        collection->vtasks,
                        store_collection->tasks_fn)
            );
        }
        ti_store_collection_destroy(store_collection);
        if (rc)
            goto failed;
    }

    (void) rename(store->store_path, store->prev_path);
    (void) ti_sleep(2);

    if (rename(store->tmp_path, store->store_path))
    {
        log_error(
                "rename from `%s` to `%s` has failed",
                store->tmp_path,
                store->store_path);
        goto failed;
    }

    (void) ti_sleep(2);
    (void) fx_rmdir(store->prev_path);
    (void) ti_sleep(2);

    store->last_stored_change_id = ti.node->ccid;

    log_info("stored thingsdb until "TI_CHANGE_ID" to: `%s`",
            store->last_stored_change_id, store->store_path);

    rc = store__collection_ids();  /* can only fail with mem allow error */

    goto done;

failed:
    rc = -1;
    log_error("storing ThingsDB has failed");
    (void) fx_rmdir(store->tmp_path);

done:
    store__set_filename(/* use_tmp: */ false);
    return rc;
}

int ti_store_restore(void)
{
    int rc;
    imap_t * namesmap;

    assert(store);

    store__set_filename(/* use_tmp: */ false);

    if (!fx_is_dir(ti.store->store_path))
    {
        log_warning(
                "store path not found: `%s`",
                ti.store->store_path);

        return ti_rebuild();
    }

    namesmap = ti_store_names_restore(store->names_fn);
    rc = (
            -(!namesmap) ||
            ti_store_status_restore(store->id_stat_fn) ||
            ti_store_users_restore(store->users_fn) ||
            ti_store_access_restore(
                    &ti.access_node,
                    store->access_node_fn) ||
            ti_store_access_restore(
                    &ti.access_thingsdb,
                    store->access_thingsdb_fn) ||
            ti_store_collections_restore(store->collections_fn) ||
            ti_store_procedures_restore(
                    ti.procedures,
                    store->procedures_fn,
                    NULL) ||
            ti_store_tasks_restore(
                    &ti.tasks->vtasks,
                    store->tasks_fn,
                    NULL) ||
            ti_store_modules_restore(store->modules_fn));

    if (rc)
        goto stop;

    for (vec_each(ti.collections->vec, ti_collection_t, collection))
    {
        ti_store_collection_t * store_collection = ti_store_collection_create(
                store->store_path,
                &collection->guid);

        /*
         * TODO: (COMPAT) This code is for compatibility with ThingsDB version
         *       before v0.9.19 and might be removed on the next major release.
         */
        if (!fx_file_exist(store_collection->gcthings_fn))
        {
            log_warning(
                    "file `%s` is missing; "
                    "create new files for garbage collect",
                    store_collection->gcthings_fn);

            (void) ti_store_gcollect_store(
                    collection->gc,
                    store_collection->gcthings_fn);
            (void) ti_store_gcollect_store_data(
                    collection->gc,
                    store_collection->gcprops_fn);
        }

        rc = (  -(!store_collection) ||
                ti_store_enums_restore(
                        collection->enums,
                        store_collection->enums_fn) ||
                ti_store_types_restore(
                        collection->types,
                        namesmap,
                        store_collection->types_fn) ||
                ti_store_access_restore(
                        &collection->access,
                        store_collection->access_fn) ||
                ti_store_things_restore(
                        collection,
                        store_collection->things_fn) ||
                ti_store_collection_restore(
                        collection,
                        store_collection->collection_fn) ||
                ti_store_enums_restore_members(
                        collection->enums,
                        namesmap,
                        store_collection->enums_fn) ||
                /*
                 * First load the things from the garbage collector; There
                 * might be data referring to things marked as garbage if
                 * the store was done before garbage collection took place.
                 */
                ti_store_gcollect_restore(
                        collection,
                        store_collection->gcthings_fn) ||
                /*
                 * Load tasks before data as task may refer to things
                 * and data may refer to tasks.
                 */
                ti_store_tasks_restore(
                        &collection->vtasks,
                        store_collection->tasks_fn,
                        collection) ||
                ti_store_things_restore_data(
                        collection,
                        namesmap,
                        store_collection->props_fn) ||
                ti_store_gcollect_restore_data(
                        collection,
                        namesmap,
                        store_collection->gcprops_fn) ||
                ti_store_named_rooms_restore(
                        store_collection->named_rooms_fn,
                        collection) ||
                ti_store_procedures_restore(
                        collection->procedures,
                        store_collection->procedures_fn,
                        collection)
        );

        ti_store_collection_destroy(store_collection);

        assert(collection->root);

        if (rc)
            goto stop;

        (void) imap_walk(
                collection->things,
                (imap_cb) store__thing_drop,
                NULL);
        /*
         * The things in the garbage collection must keep a reference,
         * therefore the garbage collection must not be walked.
         */
    }

    store->last_stored_change_id = ti.node->ccid;

    rc = store__collection_ids();  /* can only fail with mem allow error */

stop:
    if (namesmap)
        imap_destroy(namesmap, (imap_destroy_cb) ti_name_unsafe_drop);

    return rc;
}

