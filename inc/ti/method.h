/*
 * ti/method.h
 */
#ifndef TI_METHOD_H_
#define TI_MEHTOD_H_

typedef struct ti_method_s ti_method_t;

#include <ti/name.h>
#include <ti/closure.h>


struct ti_method_s
{
    ti_name_t * name;
    ti_closure_t * closure;
};

ti_method_t * ti_method_create(ti_name_t * name, ti_closure_t * closure);
void ti_method_destroy(ti_method_t * method);

#endif  /* TI_MEHTOD_H_ */
