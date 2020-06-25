/*
 * ti/method.h
 */
#ifndef TI_METHOD_T_H_
#define TI_MEHTOD_T_H_

typedef struct ti_method_s ti_method_t;

#include <ti/name.t.h>
#include <ti/closure.t.h>

struct ti_method_s
{
    ti_name_t * name;
    ti_closure_t * closure;
};

#endif  /* TI_MEHTOD_T_H_ */
