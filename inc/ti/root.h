/*
 * ti/root.h
 */
#ifndef TI_ROOT_H_
#define TI_ROOT_H_

typedef struct ti_root_s ti_root_t;
typedef union ti_root_u ti_root_via_t;

#include <cleri/cleri.h>
#include <ti/event.h>
#include <ti/val.h>
#include <ti/ex.h>
#include <ti/user.h>
#include <ti/db.h>

enum
{
    TI_ROOT_FLAG_NESTED     =1<<0,
};

ti_root_t * ti_root_create(ti_event_t * ev, ti_user_t * user);
void ti_root_destroy(ti_root_t * root);
int ti_root_scope(ti_root_t * root, cleri_node_t * nd, ex_t * e);


struct ti_root_s
{
    ti_val_t * rval;            /* must be on top (matches ti_root_t)
                                   new return value or NULL when point to
                                   scope */
    ti_event_t * ev;            /* borrowed reference or NULL if no updates are
                                   required (the query has a reference)
                                */
    ti_user_t * user;           /* borrowed user from stream */
    uint8_t flags;
};

#endif  /* TI_ROOT_H_ */
