/*
 * ti/method.c
 */

#include <ti/method.h>

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
    ti_val_drop(method->closure);
    free(method);
}
