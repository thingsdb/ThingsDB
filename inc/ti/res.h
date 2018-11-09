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
#include <cleri/cleri.h>


ti_res_t * ti_res_create(ti_db_t * db);
void ti_res_destroy(ti_res_t * res);
int ti_res_scope(ti_res_t * res, cleri_node_t * nd, ex_t * e);


struct ti_res_s
{
    ti_db_t * db;       /* this is using a borrowed reference, this is good
                           because the query has one */
    ti_val_t * rval;    /* new return value or NULL when point to scope */
    ti_scope_t * scope; /* keep the scope */
    omap_t * collect;   /* stores a vec_t with stored props to collect,
                           and the key is the thing id. */
    ti_event_t * ev;    /* NULL if no updates are required */
};


#endif /* TI_RES_H_ */
