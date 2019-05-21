/*
 * store.h
 */
#ifndef TI_STORE_H_
#define TI_STORE_H_

typedef struct ti_store_s ti_store_t;

#include <inttypes.h>

int ti_store_create(void);
void ti_store_destroy(void);
int ti_store_store(void);
int ti_store_restore(void);

struct ti_store_s
{
    char * access_node_fn;
    char * access_thingsdb_fn;
    char * collections_fn;
    char * id_stat_fn;
    char * names_fn;
    char * prev_path;
    char * store_path;
    char * tmp_path;
    char * users_fn;
    size_t fn_offset;
    uint64_t last_stored_event_id;
};

#endif /* TI_STORE_H_ */
