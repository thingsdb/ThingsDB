/*
 * ti/scope.c
 */
#include <ti.h>
#include <ti/scope.h>
#include <stdlib.h>

ti_scope_t * ti_scope_create(ti_thing_t * parent)
{
    ti_scope_t * scope = malloc(sizeof(ti_scope_t));
    if (!scope)
        return NULL;

    scope->parent = ti_grab(parent);    /* may be NULL for root or database */

    return scope;
}

void ti_scope_destroy(ti_scope_t * scope)
{
    if (!scope)
        return;
    ti_thing_drop(scope->parent);
    free(scope);
}
