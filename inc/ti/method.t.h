/*
 * ti/method.h
 */
#ifndef TI_METHOD_T_H_
#define TI_METHOD_T_H_

typedef struct ti_method_s ti_method_t;

#include <ti/closure.t.h>
#include <ti/name.t.h>
#include <ti/raw.t.h>

struct ti_method_s
{
    ti_name_t * name;               /* name of the method */
    ti_raw_t * doc;                 /* documentation, may be NULL */
    ti_raw_t * def;                 /* formatted definition, may be NULL */
    ti_closure_t * closure;         /* closure */
};

#endif  /* TI_METHOD_T_H_ */
