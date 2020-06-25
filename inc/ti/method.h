/*
 * ti/method.h
 */
#ifndef TI_METHOD_H_
#define TI_MEHTOD_H_

#include <ti/method.t.h>
#include <ti/name.t.h>
#include <ti/closure.t.h>

ti_method_t * ti_method_create(ti_name_t * name, ti_closure_t * closure);
void ti_method_destroy(ti_method_t * method);

#endif  /* TI_MEHTOD_H_ */
