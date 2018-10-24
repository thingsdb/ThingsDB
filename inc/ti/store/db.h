/*
 * ti/store/db.h
 */

typedef struct ti_store_db_s ti_store_db_t;

#include <ti/db.h>

ti_store_db_t * ti_store_db_create(const char * path, ti_db_t * db);
void ti_store_db_destroy(ti_store_db_t * store_db);

struct ti_store_db_s
{
    char * access_fn;
    char * data_fn;
    char * db_fn;
    char * db_path;
    char * names_fn;
    char * skeleton_fn;
    char * things_fn;
};
