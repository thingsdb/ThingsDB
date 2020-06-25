/*
 * ti/collection.c
 */
#include <assert.h>
#include <doc.h>
#include <stdlib.h>
#include <string.h>
#include <ti.h>
#include <ti/auth.h>
#include <ti/collection.h>
#include <ti/collection.inline.h>
#include <ti/enums.h>
#include <ti/name.h>
#include <ti/name.h>
#include <ti/names.h>
#include <ti/procedure.h>
#include <ti/raw.inline.h>
#include <ti/thing.h>
#include <ti/things.h>
#include <ti/types.h>
#include <ti/val.inline.h>
#include <util/fx.h>
#include <util/strx.h>


static const size_t ti_collection_min_name = 1;
static const size_t ti_collection_max_name = 128;

ti_collection_t * ti_collection_create(
        guid_t * guid,
        const char * name,
        size_t n,
        uint64_t created_at)
{
    ti_collection_t * collection = malloc(sizeof(ti_collection_t));
    if (!collection)
        return NULL;

    collection->ref = 1;
    collection->root = NULL;
    collection->name = ti_str_create(name, n);
    collection->things = imap_create();
    collection->access = vec_new(1);
    collection->procedures = vec_new(0);
    collection->types = ti_types_create(collection);
    collection->enums = ti_enums_create(collection);
    collection->lock = malloc(sizeof(uv_mutex_t));
    collection->created_at = created_at;

    memcpy(&collection->guid, guid, sizeof(guid_t));

    if (!collection->name || !collection->things || !collection->access ||
        !collection->procedures || !collection->lock ||!collection->types ||
        !collection->enums || uv_mutex_init(collection->lock))
    {
        ti_collection_drop(collection);
        return NULL;
    }

    return collection;
}

void ti_collection_destroy(ti_collection_t * collection)
{
    if (!collection)
        return;

    imap_destroy(collection->things, NULL);
    ti_val_drop((ti_val_t *) collection->name);
    vec_destroy(collection->access, (vec_destroy_cb) ti_auth_destroy);
    vec_destroy(collection->procedures, (vec_destroy_cb) ti_procedure_destroy);
    ti_types_destroy(collection->types);
    ti_enums_destroy(collection->enums);
    uv_mutex_destroy(collection->lock);
    free(collection->lock);
    free(collection);
}

void ti_collection_drop(ti_collection_t * collection)
{
    if (!collection || --collection->ref)
        return;

    ti_val_drop((ti_val_t *) collection->root);

    if (!collection->things->n)
    {
        ti_collection_destroy(collection);
        return;
    }

    if (ti_collections_add_for_collect(collection))
        log_critical(EX_MEMORY_S);
}

_Bool ti_collection_name_check(const char * name, size_t n, ex_t * e)
{
    if (n < ti_collection_min_name || n >= ti_collection_max_name)
    {
        ex_set(e, EX_VALUE_ERROR,
                "collection name must be between %u and %u characters",
                ti_collection_min_name,
                ti_collection_max_name);
        return false;
    }

    if (!ti_name_is_valid_strn(name, n))
    {
        ex_set(e, EX_VALUE_ERROR,
                "collection name must follow the naming rules"
                DOC_NAMES);
        return false;
    }

    return true;
}

int ti_collection_rename(
        ti_collection_t * collection,
        ti_raw_t * rname,
        ex_t * e)
{
    if (!ti_name_is_valid_strn((const char *) rname->data, rname->n))
    {
        ex_set(e, EX_VALUE_ERROR,
                "collection name must follow the naming rules"
                DOC_NAMES);
        return -1;
    }

    if (ti_collections_get_by_strn((const char *) rname->data, rname->n))
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "collection `%.*s` already exists",
                (int) rname->n, (const char *) rname->data);
        return -1;
    }

    ti_val_drop((ti_val_t *) collection->name);
    collection->name = ti_grab(rname);

    return 0;
}

ti_val_t * ti_collection_as_mpval(ti_collection_t * collection)
{
    ti_raw_t * raw;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_raw_t), sizeof(ti_raw_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (ti_collection_to_pk(collection, &pk))
    {
        msgpack_sbuffer_destroy(&buffer);
        return NULL;
    }

    raw = (ti_raw_t *) buffer.data;
    ti_raw_init(raw, TI_VAL_MP, buffer.size);

    return (ti_val_t *) raw;
}
