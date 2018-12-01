/*
 * ti/res.h
 */
#ifndef TI_RES_H_
#define TI_RES_H_

typedef struct ti_res_s ti_res_t;

#include <ti/db.h>
#include <ti/val.h>
#include <ti/ex.h>
#include <ti/event.h>
#include <ti/scope.h>
#include <util/omap.h>
#include <util/vec.h>
#include <cleri/cleri.h>

ti_res_t * ti_res_create(
        ti_db_t * db,
        ti_event_t * ev,
        vec_t * blobs,
        vec_t * nd_cache,
        omap_t * collect);
void ti_res_destroy(ti_res_t * res);
int ti_res_scope(ti_res_t * res, cleri_node_t * nd, ex_t * e);

struct ti_res_s
{
    ti_val_t * rval;            /* must be on top (matches ti_root_t)
                                   new return value or NULL when point to
                                   scope
                                */
    ti_db_t * db;               /* borrowed reference, the query has one */
    ti_event_t * ev;            /* borrowed reference or NULL if no updates are
                                   required (the query has a reference)
                                */
    vec_t * blobs;              /* pointer to query->blobs, may be NULL */
    vec_t * nd_cache;           /* pointer to query->nd_cache, may be NULL */
    omap_t * collect;           /* pointer to query->collect */
    ti_scope_t * scope;         /* keep the scope */
};


#endif /* TI_RES_H_ */
