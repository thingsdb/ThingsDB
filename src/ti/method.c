/*
 * ti/method.c
 */
#include <tiinc.h>
#include <ti/method.h>
#include <ti/name.h>
#include <ti/closure.t.h>
#include <ti/val.h>
#include <ti/val.inline.h>

ti_method_t * ti_method_create(ti_name_t * name, ti_closure_t * closure)
{
    ti_method_t * method = malloc(sizeof(ti_method_t));
    if (!method)
        return NULL;
    method->name = name;
    method->closure = closure;

    ti_incref(name);
    ti_incref(closure);

    return method;
}

void ti_method_destroy(ti_method_t * method)
{
    ti_name_drop(method->name);
    ti_val_drop((ti_val_t *) method->closure);
    free(method);
}
