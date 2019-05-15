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
#include <qpack.h>
#include <ti/ex.h>
#include <ti/val.h>

int ti_collections_create(void);
void ti_collections_destroy(void);
void ti_collections_clear(void);
int ti_collections_gc(void);
_Bool ti_collections_del_collection(const uint64_t collection_id);
int ti_collections_add_for_collect(imap_t * things);
int ti_collections_gc_collect_dropped(void);
ti_collection_t * ti_collections_create_collection(
        uint64_t root_id,   /* when 0, a new thing id will be generated */
        const char * name,
        size_t n,
        ti_user_t * user,
        ex_t * e);
ti_collection_t * ti_collections_get_by_raw(const ti_raw_t * raw);
ti_collection_t * ti_collections_get_by_strn(const char * str, size_t n);
ti_collection_t * ti_collections_get_by_id(const uint64_t id);
ti_collection_t * ti_collections_get_by_qp_obj(
        qp_obj_t * obj,
        _Bool allow_root,
        ex_t * e);
ti_collection_t * ti_collections_get_by_val(
        ti_val_t * val,
        _Bool allow_root,
        ex_t * e);
void ti_collections_get(ti_stream_t * sock, ti_pkg_t * pkg, ex_t * e);
int ti_collections_to_packer(qp_packer_t ** packer);
ti_val_t * ti_collections_as_qpval(void);

struct ti_collections_s
{
    vec_t * vec;        /* ti_collection_t with reference */
    vec_t * dropped;    /* imap_t with things (for garbage collection) */
};

#endif /* TI_COLLECTIONS_H_ */
