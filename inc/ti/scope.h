/*
 * ti/scope.h
 */
#ifndef TI_SCOPE_H_
#define TI_SCOPE_H_

typedef struct ti_scope_s ti_scope_t;

#include <ti/val.h>

ti_scope_t * ti_scope_create(ti_thing_t * parent);
void ti_scope_destroy(ti_scope_t * scope);


struct ti_scope_s
{
    ti_val_t * parent;        /* NULL means root or database */
};

#endif /* TI_SCOPE_H_ */
