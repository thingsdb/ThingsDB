/*
 * ti/method.h
 */
#ifndef TI_METHOD_H_
#define TI_MEHTOD_H_

#include <cleri/cleri.h>
#include <ex.h>
#include <ti/closure.t.h>
#include <ti/method.t.h>
#include <ti/name.t.h>
#include <ti/query.t.h>
#include <ti/raw.t.h>
#include <ti/type.t.h>

ti_method_t * ti_method_create(ti_name_t * name, ti_closure_t * closure);
void ti_method_destroy(ti_method_t * method);
ti_method_t * ti_method_by_name(ti_type_t * type, ti_name_t * name);
ti_raw_t * ti_method_doc(ti_method_t * method);
ti_raw_t * ti_method_def(ti_method_t * method);
int ti_method_call(
        ti_method_t * method,
        ti_type_t * type,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e);
int ti_method_set_name(
        ti_method_t * method,
        ti_type_t * type,
        const char * s,
        size_t n,
        ex_t * e);

#endif  /* TI_MEHTOD_H_ */
