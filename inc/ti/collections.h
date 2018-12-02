/*
 * collections.h
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
ti_collection_t * ti_collections_create_collection(
        const char * name,
        size_t n,
        ti_user_t * user,
        ex_t * e);
ti_collection_t * ti_collections_get_by_raw(const ti_raw_t * raw);
ti_collection_t * ti_collections_get_by_strn(const char * str, size_t n);
ti_collection_t * ti_collections_get_by_id(const uint64_t id);
ti_collection_t * ti_collections_get_by_qp_obj(qp_obj_t * obj, ex_t * e);
ti_collection_t * ti_collections_get_by_val(ti_val_t * val, ex_t * e);
void ti_collections_get(ti_stream_t * sock, ti_pkg_t * pkg, ex_t * e);

struct ti_collections_s
{
    vec_t * vec;        /* ti_collection_t with reference */
};

#endif /* TI_COLLECTIONS_H_ */
