/*
 * ti/query.h
 */
#ifndef TI_QUERY_H_
#define TI_QUERY_H_

#include <ex.h>
#include <ti/name.t.h>
#include <ti/prop.t.h>
#include <ti/qbind.t.h>
#include <ti/query.t.h>
#include <ti/scope.t.h>
#include <ti/type.t.h>
#include <ti/user.t.h>
#include <ti/vup.t.h>

ti_query_t * ti_query_create(void * via, ti_user_t * user, uint8_t flags);
void ti_query_destroy(ti_query_t * query);
int ti_query_unpack(
        ti_query_t * query,
        ti_scope_t * scope,
        uint16_t pkg_id,
        const unsigned char * data,
        size_t n,
        ex_t * e);
int ti_query_unp_run(
        ti_query_t * query,
        ti_scope_t * scope,
        uint16_t pkg_id,
        const unsigned char * data,
        size_t n,
        ex_t * e);
int ti_query_parse(ti_query_t * query, ex_t * e);
int ti_query_investigate(ti_query_t * query, ex_t * e);
void ti_query_run(ti_query_t * query);
void ti_query_send_response(ti_query_t * query, ex_t * e);
int ti_query_unpack_args(ti_query_t * query, ti_vup_t * vup, ex_t * e);
int ti_query_apply_scope(ti_query_t * query, ti_scope_t * scope, ex_t * e);
ti_prop_t * ti_query_var_get(ti_query_t * query, ti_name_t * name);
ti_thing_t * ti_query_thing_from_id(
        ti_query_t * query,
        int64_t thing_id,
        ex_t * e);
ssize_t ti_query_count_type(ti_query_t * query, ti_type_t * type);
static inline _Bool ti_query_will_update(ti_query_t * query);
static inline const char * ti_query_scope_name(ti_query_t * query);
int ti_query_vars_walk(vec_t * vars, imap_cb cb, void * args);

static inline _Bool ti_query_will_update(ti_query_t * query)
{
    return query->qbind.flags & TI_QBIND_FLAG_EVENT;
}

static inline const char * ti_query_scope_name(ti_query_t * query)
{
    return query->qbind.flags & TI_QBIND_FLAG_NODE
            ? "@node"
            : query->qbind.flags & TI_QBIND_FLAG_THINGSDB
            ? "@thingsdb"
            : query->qbind.flags & TI_QBIND_FLAG_COLLECTION
            ? "@collection"
            : "<unknown>";
}
#endif /* TI_QUERY_H_ */
