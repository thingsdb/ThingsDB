/*
 * ti/store/collection.h
 */

typedef struct ti_store_collection_s ti_store_collection_t;

#include <ti/collection.h>

ti_store_collection_t * ti_store_collection_create(
        const char * path,
        ti_collection_t * collection);
void ti_store_collection_destroy(ti_store_collection_t * store_collection);
int ti_store_collection_store(ti_collection_t * collection, const char * fn);
int ti_store_collection_restore(ti_collection_t * collection, const char * fn);


struct ti_store_collection_s
{
    char * access_fn;
    char * data_fn;
    char * collection_fn;
    char * collection_path;
    char * names_fn;
    char * skeleton_fn;
    char * things_fn;
};
