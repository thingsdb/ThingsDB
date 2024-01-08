/*
 * ti/collections.h
 */
#ifndef TI_COLLECTIONS_H_
#define TI_COLLECTIONS_H_

typedef struct ti_collections_s ti_collections_t;

#include <ti/collection.h>
#include <ti/pkg.h>
#include <ti/raw.h>
#include <ti/stream.h>
#include <ti/user.h>
#include <ex.h>
#include <ti/val.h>

int ti_collections_create(void);
void ti_collections_destroy(void);
void ti_collections_clear(void);
int ti_collections_gc(void);
_Bool ti_collections_del_collection(const uint64_t collection_id);
int ti_collections_add_for_collect(ti_collection_t * collection);
int ti_collections_gc_collect_dropped(void);
ti_collection_t * ti_collections_create_collection(
        uint64_t collection_id,  /* when 0, a new thing id will be generated */
        uint64_t next_free_id,
        const char * name,
        size_t name_n,
        uint64_t created_at,
        ti_user_t * user,
        ex_t * e);
ti_collection_t * ti_collections_get_by_strn(const char * str, size_t n);
ti_collection_t * ti_collections_get_by_id(const uint64_t id);
ti_collection_t * ti_collections_get_by_val(ti_val_t * val, ex_t * e);
ti_varr_t * ti_collections_info(ti_user_t * user);

struct ti_collections_s
{
    vec_t * vec;        /* ti_collection_t with reference */
    vec_t * dropped;    /* imap_t with things (for garbage collection) */
};

#endif /* TI_COLLECTIONS_H_ */
