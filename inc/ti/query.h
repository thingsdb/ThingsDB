/*
 * ti/query.h
 */
#ifndef TI_QUERY_H_
#define TI_QUERY_H_

#include <ex.h>
#include <ti/collection.t.h>
#include <ti/future.t.h>
#include <ti/name.t.h>
#include <ti/prop.t.h>
#include <ti/qbind.t.h>
#include <ti/query.t.h>
#include <ti/room.t.h>
#include <ti/scope.t.h>
#include <ti/type.t.h>
#include <ti/user.t.h>
#include <ti/vup.t.h>

extern ti_query_done_cb ti_query_done_map[];
extern ti_query_run_cb ti_query_run_map[];

ti_query_t * ti_query_create(uint8_t flags);
void ti_query_destroy(ti_query_t * query);
int ti_query_unp_run(
        ti_query_t * query,
        ti_scope_t * scope,
        uint16_t pkg_id,
        const unsigned char * data,
        size_t n,
        ex_t * e);
int ti_query_parse(ti_query_t * query, const char * str, size_t n, ex_t * e);
void ti_query_run_parseres(ti_query_t * query);
void ti_query_run_procedure(ti_query_t * query);
void ti_query_run_future(ti_query_t * query);
void ti_query_run_task_finish(ti_query_t * query);
void ti_query_run_task(ti_query_t * query);
void ti_query_send_response(ti_query_t * query, ex_t * e);
void ti_query_on_then_result(ti_query_t * query, ex_t * e);
void ti_query_task_result(ti_query_t * query, ex_t * e);
void ti_query_done(ti_query_t * query, ex_t * e, ti_query_done_cb cb);
void ti_query_on_future_result(ti_future_t * future, ex_t * e);
int ti_query_unpack_args(ti_query_t * query, mp_unp_t * up, ex_t * e);
int ti_query_apply_scope(ti_query_t * query, ti_scope_t * scope, ex_t * e);
ti_prop_t * ti_query_var_get(ti_query_t * query, ti_name_t * name);
ti_thing_t * ti_query_thing_from_id(
        ti_query_t * query,
        int64_t thing_id,
        ex_t * e);
ti_room_t * ti_query_room_from_id(
        ti_query_t * query,
        int64_t room_id,
        ex_t * e);
ti_room_t * ti_query_room_from_strn(
        ti_query_t * query,
        const char * str,
        size_t n,
        ex_t * e);
ssize_t ti_query_count_type(ti_query_t * query, ti_type_t * type);
int ti_query_vars_walk(
        vec_t * vars,
        ti_collection_t * collection,
        imap_cb cb,
        void * args);
int ti_query_task_context(ti_query_t * query, ti_vtask_t * vtask, ex_t * e);
_Bool ti_query_thing_can_change_spec(ti_query_t * query, ti_thing_t * thing);
void ti_query_warn_log(ti_query_t * query, const char * msg);

static inline _Bool ti_query_wse(ti_query_t * query)
{
    return query->qbind.flags & TI_QBIND_FLAG_WSE;
}

static inline const char * ti_query_scope_name(ti_query_t * query)
{
    return query->qbind.flags & TI_QBIND_FLAG_THINGSDB
            ? "@thingsdb"
            : query->qbind.flags & TI_QBIND_FLAG_COLLECTION
            ? "@collection"
            : "@node";
}

#endif  /* TI_QUERY_H_ */
