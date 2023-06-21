/*
 * ti/method.h
 */
#ifndef TI_METHOD_H_
#define TI_MEHTOD_H_

#include <cleri/cleri.h>
#include <ex.h>
#include <ti/closure.t.h>
#include <ti/enum.t.h>
#include <ti/member.t.h>
#include <ti/method.t.h>
#include <ti/name.t.h>
#include <ti/query.t.h>
#include <ti/raw.t.h>
#include <ti/type.t.h>

ti_method_t * ti_method_create(ti_name_t * name, ti_closure_t * closure);
void ti_method_destroy(ti_method_t * method);
ti_raw_t * ti_method_doc(ti_method_t * method);
ti_raw_t * ti_method_def(ti_method_t * method);
int ti_method_call(
        ti_method_t * method,
        ti_type_t * type_or_enum,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e);
int ti_method_call_on_member(
        ti_method_t * method,
        ti_member_t * member,
        ti_query_t * query,
        cleri_node_t * nd,
        ex_t * e);
int ti_method_set_name_t(
        ti_method_t * method,
        ti_type_t * type,
        const char * s,
        size_t n,
        ex_t * e);
int ti_method_set_name_e(
        ti_method_t * method,
        ti_enum_t * enum_,
        const char * s,
        size_t n,
        ex_t * e);
void ti_method_set_closure(ti_method_t * method, ti_closure_t * closure);

#endif  /* TI_MEHTOD_H_ */
