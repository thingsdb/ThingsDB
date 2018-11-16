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
#include <ti/db.h>

enum
{
    TI_ROOT_UNDEFINED,
    TI_ROOT_DATABASES,
    TI_ROOT_USERS,
    TI_ROOT_NODES,
    TI_ROOT_CONFIG,
    TI_ROOT_DATABASE,
};

ti_root_t * ti_root_create(void);
void ti_root_destroy(ti_root_t * root);
int ti_root_scope(ti_root_t * root, cleri_node_t * nd, ex_t * e);

union ti_root_u
{
    ti_db_t * db;
};

struct ti_root_s
{
    ti_val_t * rval;            /* must be on top (matches ti_root_t)
                                   new return value or NULL when point to
                                   scope */
    ti_event_t * ev;            /* NULL if no updates are required */
    uint8_t tp;
    ti_root_via_t via;
};

#endif  /* TI_ROOT_H_ */
