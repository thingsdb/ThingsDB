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


ti_res_t * ti_res_create(ti_db_t * db, vec_t * blobs);
void ti_res_destroy(ti_res_t * res);
int ti_res_scope(ti_res_t * res, cleri_node_t * nd, ex_t * e);


struct ti_res_s
{
    vec_t * blobs;              /* pointer to query->blobs, may be NULL */
    ti_val_t * rval;            /* must be on top (matches ti_root_t)
                                   new return value or NULL when point to
                                   scope */
    ti_db_t * db;               /* this is using a borrowed reference, this is
                                   good because the query has one */
    ti_scope_t * scope;         /* keep the scope */
    omap_t * collect;           /* contains a vec_t with attributes to collect,
                                   and the key is the thing id. */
    ti_event_t * ev;            /* NULL if no updates are required */
};


#endif /* TI_RES_H_ */
