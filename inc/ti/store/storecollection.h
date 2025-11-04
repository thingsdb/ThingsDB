/*
 * ti/store/collection.h
 */

typedef struct ti_store_collection_s ti_store_collection_t;

#include <ti/collection.h>
#include <util/guid.h>


ti_store_collection_t * ti_store_collection_create(
        const char * path,
        guid_t * guid);
void ti_store_collection_destroy(ti_store_collection_t * store_collection);
int ti_store_collection_store(ti_collection_t * collection, const char * fn);
int ti_store_collection_restore(ti_collection_t * collection, const char * fn);
_Bool ti_store_collection_is_stored(const char * path, uint64_t collection_id);
char * ti_store_collection_get_path(const char * path, uint64_t collection_id);
char * ti_store_collection_access_fn(
        const char * path,
        uint64_t collection_id);
char * ti_store_collection_procedures_fn(
        const char * path,
        uint64_t collection_id);
char * ti_store_collection_tasks_fn(
        const char * path,
        uint64_t collection_id);
char * ti_store_collection_dat_fn(
        const char * path,
        uint64_t collection_id);
char * ti_store_collection_props_fn(
        const char * path,
        uint64_t collection_id);
char * ti_store_collection_things_fn(
        const char * path,
        uint64_t collection_id);
char* ti_store_collection_types_fn(
        const char * path,
        uint64_t collection_id);
char * ti_store_collection_enums_fn(
        const char * path,
        uint64_t collection_id);
char * ti_store_collection_gcprops_fn(
        const char * path,
        uint64_t collection_id);
char * ti_store_collection_gcthings_fn(
        const char * path,
        uint64_t collection_id);
char * ti_store_collection_named_rooms_fn(
        const char * path,
        uint64_t collection_id);
char * ti_store_collection_commits_fn(
        const char * path,
        uint64_t collection_id);

struct ti_store_collection_s
{
    char * access_fn;
    char * collection_fn;
    char * props_fn;
    char * collection_path;
    char * commits_fn;
    char * names_fn;
    char * named_rooms_fn;
    char * procedures_fn;
    char * tasks_fn;
    char * things_fn;
    char * types_fn;
    char * enums_fn;
    char * gcprops_fn;
    char * gcthings_fn;
};
