/*
 * task.c
 */
#include <stdlib.h>
#include <assert.h>
#include <dbs.h>
#include <props.h>
#include <ti/access.h>
#include <ti/api.h>
#include <ti/auth.h>
#include <ti/prop.h>
#include <ti/proto.h>
#include <ti/task.h>
#include <ti/thing.h>
#include <users.h>
#include <util/qpx.h>
//
//static ti_task_stat_e ti__task_user_create(
//        qp_map_t * task,
//        ti_event_t * event);
//static ti_task_stat_e ti__task_db_create(
//        qp_map_t * task,
//        ti_event_t * event);
//static ti_task_stat_e ti__task_grant(
//        qp_map_t * task,
//        ti_event_t * event);
//static ti_task_stat_e ti__task_props_set(
//        qp_map_t * task,
//        ti_event_t * event);
//static ti_task_stat_e ti__task_props_del(
//        qp_map_t * task,
//        ti_event_t * event);
//static inline ti_thing_t * ti__task_thing_by_id(
//        ti_event_t * event,
//        int64_t sid);
//static ti_thing_t * ti__task_thing_create(ti_event_t * event, int64_t sid);
//static inline ti_task_stat_e ti__task_fail(
//        ti_event_t * event,
//        const char * msg);
//static ti_task_stat_e ti__task_failn(
//        ti_event_t * event,
//        const char * msg,
//        size_t n);
//
//ti_task_t * ti_task_create(qp_res_t * res, ex_t * e)
//{
//    ti_task_t * task;
//    qp_res_t * tasktp;
//    if (res->tp != QP_RES_MAP)
//    {
//        ex_set(e, TI_PROTO_TYPE_ERR, "expecting each task to be a map");
//        return NULL;
//    }
//    tasktp = qpx_map_get(res->via.map, TI_API_TASK);
//    if (!tasktp ||
//        tasktp->tp != QP_RES_INT64 ||
//        tasktp->via.int64 < 0  ||
//        tasktp->via.int64 > TI_TASK_N)
//    {
//        ex_set(e, TI_PROTO_TYPE_ERR,
//                "invalid or missing task type ("TI_API_TASK")");
//        return NULL;
//    }
//
//    task = (ti_task_t *) malloc(sizeof(ti_task_t));
//    if (!task)
//    {
//        ex_set_alloc(e);
//        return NULL;
//    }
//
//    task->tp = (ti_task_e) tasktp->via.int64;
//    task->res = res;
//
//    return task;
//}
//
//void ti_task_destroy(ti_task_t * task)
//{
//    if (!task) return;
//    if (task->res)
//    {
//        qp_res_destroy(task->res);
//    }
//    free(task);
//}
//
//ti_task_stat_e ti_task_run(
//        ti_task_t * task,
//        ti_event_t * event,
//        ti_task_stat_e rc)
//{
//    if (event->client && qp_add_map(&event->result))
//    {
//        rc = TI_TASK_ERR;
//        goto finish;
//    }
//    if (rc != TI_TASK_SUCCESS)
//    {
//        rc = TI_TASK_SKIPPED;
//        goto finish;
//    }
//    qp_map_t * map = task->res->via.map;
//    switch (task->tp)
//    {
//    case TI_TASK_USER_CREATE:
//        rc = ti__task_user_create(map, event);
//        break;
//    case TI_TASK_DB_CREATE:
//        rc = ti__task_db_create(map, event);
//        break;
//    case TI_TASK_GRANT:
//        rc = ti__task_grant(map, event);
//        break;
//    case TI_TASK_PROPS_SET:
//        rc = ti__task_props_set(map, event);
//        break;
//    case TI_TASK_PROPS_DEL:
//        rc = ti__task_props_del(map, event);
//        break;
//    default:
//        assert (0);
//    }
//
//finish:
//    if ((event->client && qp_add_raw_from_str(event->result, TI_API_STAT)) ||
//        qp_add_int64(event->result, rc) ||
//        (event->client && qp_close_map(event->result))) return TI_TASK_ERR;
//
//    return rc;
//}
//
//const char * ti_task_str(ti_task_t * task)
//{
//    switch(task->tp)
//    {
//    case TI_TASK_USER_CREATE: return "TASK_USER_CREATE";
//    case TI_TASK_USER_DROP: return "TASK_USER_DROP";
//    case TI_TASK_USER_ALTER: return "TASK_USER_ALTER";
//    case TI_TASK_DB_CREATE: return "TASK_DB_CREATE";
//    case TI_TASK_DB_RENAME: return "TASK_DB_RENAME";
//    case TI_TASK_DB_DROP: return "TASK_DB_DROP";
//    case TI_TASK_GRANT: return "TASK_GRANT";
//    case TI_TASK_REVOKE: return "TASK_REVOKE";
//    case TI_TASK_SET_REDUNDANCY: return "TASK_SET_REDUNDANCY";
//    case TI_TASK_NODE_ADD: return "TASK_NODE_ADD";
//    case TI_TASK_NODE_REPLACE: return "TASK_NODE_REPLACE";
//    case TI_TASK_SUBSCRIBE: return "TASK_SUBSCRIBE";
//    case TI_TASK_UNSUBSCRIBE: return "TASK_UNSUBSCRIBE";
//    case TI_TASK_PROPS_SET: return "TASK_PROPS_SET";
//    case TI_TASK_PROPS_DEL: return "TASK_PROPS_DEL";
//    default:
//        return "TASK_UNKNOWN";
//    }
//}
//
//static ti_task_stat_e ti__task_user_create(
//        qp_map_t * task,
//        ti_event_t * event)
//{
//    ex_t * e = ex_use();
//    if (event->target)
//    {
//        ex_set(e, -1, "users can only be created on tin: '%s'", ti_name);
//        return ti__task_fail(event, e->msg);
//    }
//    ti_t * tin = event->events->tin;
//    qp_res_t * user, * pass;
//    ti_user_t * usr;
//    user = qpx_map_get(task, TI_API_USER);
//    pass = qpx_map_get(task, TI_API_PASS);
//    if (!user || user->tp != QP_RES_RAW ||
//        !pass || pass->tp != QP_RES_RAW)
//    {
//        return ti__task_fail(event,
//                "missing or invalid user ("TI_API_USER") "
//                "or password ("TI_API_PASS")");
//    }
//
//    if (ti_user_name_check(user->via.raw, e) ||
//        ti_user_pass_check(pass->via.raw, e))
//    {
//        return ti__task_fail(event, e->msg);
//    }
//
//    if (ti_users_get_by_name(tin->users, user->via.raw))
//    {
//        return ti__task_fail(event, "user already exists");
//    }
//
//    char * passstr = ti_raw_to_str(pass->via.raw);
//    if (!passstr)
//    {
//        log_critical(EX_ALLOC);
//        return TI_TASK_ERR;
//    }
//
//    usr = ti_user_create(
//            ti_get_id(tin),
//            user->via.raw,
//            passstr);
//
//    free(passstr);
//
//    if (!usr ||
//        ti_user_set_pass(usr, usr->pass) ||
//        vec_push(&tin->users, usr))
//    {
//        log_critical(EX_ALLOC);
//        ti_user_drop(usr);
//        return TI_TASK_ERR;
//    }
//
//    return TI_TASK_SUCCESS;
//}
//
//static ti_task_stat_e ti__task_db_create(
//        qp_map_t * task,
//        ti_event_t * event)
//{
//    ex_t * e = ex_use();
//    if (event->target)
//    {
//        ex_set(e, -1, "databases can only be created on tin: '%s'", ti_name);
//        return ti__task_fail(event, e->msg);
//    }
//    ti_t * tin = event->events->tin;
//    qp_res_t * quser, * qname;
//    ti_user_t * user;
//    ti_db_t * db;
//    quser = qpx_map_get(task, TI_API_USER);
//    qname = qpx_map_get(task, TI_API_NAME);
//    if (!quser || quser->tp != QP_RES_RAW ||
//        !qname || qname->tp != QP_RES_RAW)
//    {
//        return ti__task_fail(event,
//                "missing or invalid user ("TI_API_USER") "
//                "or database name ("TI_API_NAME")");
//    }
//
//    if (ti_db_name_check(qname->via.raw, e))
//    {
//        return ti__task_fail(event, e->msg);
//    }
//
//    if (ti_dbs_get_by_name(tin->dbs, qname->via.raw))
//    {
//        return ti__task_fail(event, "database already exists");
//    }
//
//    user = ti_users_get_by_name(tin->users, quser->via.raw);
//    if (!user) return ti__task_fail(event, "user not found");
//
//    guid_t guid;
//    guid_init(&guid, ti_get_id(tin));
//
//    db = ti_db_create(&guid, qname->via.raw);
//    if (!db || vec_push(&tin->dbs, db))
//    {
//        log_critical(EX_ALLOC);
//        ti_db_drop(db);
//        return TI_TASK_ERR;
//    }
//
//    if (ti_access_grant(&db->access, user, TI_AUTH_MASK_FULL) ||
//        ti_db_buid(db) ||
//        qp_add_raw_from_str(event->result, TI_API_ID) ||
//        qp_add_int64(event->result, (int64_t) db->root->id))
//    {
//        log_critical(EX_ALLOC);
//        return TI_TASK_ERR;
//    }
//
//    return TI_TASK_SUCCESS;
//}
//
///*
// * Grant permissions to either a database or tin.
// */
//static ti_task_stat_e ti__task_grant(
//        qp_map_t * task,
//        ti_event_t * event)
//{
//    qp_res_t * quser = qpx_map_get(task, TI_API_USER);
//    qp_res_t * qperm = qpx_map_get(task, TI_API_PERM);
//
//    if (!quser || quser->tp != QP_RES_RAW ||
//        !qperm || qperm->tp != QP_RES_INT64)
//    {
//        return ti__task_fail(event,
//                "missing or invalid user ("TI_API_USER") "
//                "or permission flags ("TI_API_PERM")");
//    }
//
//    ti_user_t * user = ti_users_get_by_name(
//            event->events->tin->users,
//            quser->via.raw);
//
//    if (!user) return ti__task_fail(event, "user not found");
//
//    if (ti_access_grant(
//            (event->target) ?
//                    &event->target->access : &event->events->tin->access,
//            user,
//            (uint64_t) qperm->via.int64))
//    {
//        log_critical(EX_ALLOC);
//        return TI_TASK_ERR;
//    }
//
//    return TI_TASK_SUCCESS;
//}
//
//
///*
// * Grant permissions to either a database or tin.
// */
//static ti_task_stat_e ti__task_props_set(
//        qp_map_t * task,
//        ti_event_t * event)
//{
//    ex_t * e = ex_use();
//    ti_t * tin = event->events->tin;
//    ti_db_t * db = event->target;
//    ti_thing_t * thing;
//    if (!event->target)
//    {
//        return ti__task_fail(event,
//            "props can only be set on things in a database");
//    }
//    qp_res_t * qid = qpx_map_get(task, TI_API_ID);
//
//    if (!qid || qid->tp != QP_RES_INT64)
//    {
//        return ti__task_fail(event,
//                "missing or invalid thing id ("TI_API_ID")");
//    }
//
//    uint64_t id = (uint64_t) qid->via.int64;
//    if (qid->via.int64 < 0)
//    {
//        thing = ti__task_thing_create(event, id);
//        if (!thing) return TI_TASK_ERR;
//    }
//    else
//    {
//        thing = (ti_thing_t *) imap_get(db->things, id);
//        if (!thing)
//        {
//            ex_set(e, -1, "cannot find thing: %"PRIu64, id);
//            return ti__task_fail(event, e->msg);
//        }
//    }
//
//    _Bool has_thing = ti_has_id(tin, thing->id);
//
//    void * val;
//    ti_val_e tp;
//
//    qp_res_t * k, * v;
//    for (size_t i = 0; i < task->n; i++)
//    {
//        k = task->keys + i;
//        if (!*k->via.str || *k->via.str == TI_API_PREFIX[0]) continue;
//
//        ti_prop_t * prop = ti_db_props_get(db->props, k->via.str);
//        if (!prop) return TI_TASK_ERR;
//
//        v = task->values + i;
//        if (ti_thing_res_is_id(v))
//        {
//            int64_t sid = v->via.map->values[0].via.int64;
//            ti_thing_t * el = ti__task_thing_by_id(event, sid);
//            if (!el)
//            {
//                ex_set(e, -1, "cannot find thing: %"PRId64, sid);
//                return ti__task_fail(event, e->msg);
//            }
//            if (ti_thing_set(thing, prop, TI_VAL_ELEM, el))
//            {
//                log_critical(EX_ALLOC);
//                return TI_TASK_ERR;
//            }
//            continue;
//        }
//        switch (v->tp)
//        {
//        case QP_RES_MAP:
//            return ti__task_fail(event,
//                    "map must be an thing {\""TI_API_ID"\": <id>}");
//
//        case QP_RES_ARRAY:
//            assert(0);  /* TODO: fix arrays */
//            continue;
//        case QP_RES_INT64:
//            val = &v->via.int64;
//            tp = TI_VAL_INT;
//            break;
//        case QP_RES_REAL:
//            val = &v->via.real;
//            tp = TI_VAL_FLOAT;
//            break;
//        case QP_RES_RAW:
//            val = (void *) v->via.raw;
//            v->via.raw = NULL;
//            tp = TI_VAL_RAW;
//            break;
//        case QP_RES_BOOL:
//            val = &v->via.boolean;
//            tp = TI_VAL_BOOL;
//            break;
//        case QP_RES_NULL:
//            val = NULL;
//            tp = TI_VAL_NIL;
//            break;
//        case QP_RES_STR:
//            assert (0);
//            /* no break */
//        default:
//            assert (0);
//            break;
//        }
//        if (has_thing && ti_thing_weak_set(thing, prop, tp, val))
//        {
//            log_critical(EX_ALLOC);
//            return TI_TASK_ERR;
//        }
//    }
//
//    if (qp_add_raw_from_str(event->result, TI_API_ID) ||
//        qp_add_int64(event->result, (int64_t) thing->id))
//    {
//        log_critical(EX_ALLOC);
//        return TI_TASK_ERR;
//    }
//
//    return TI_TASK_SUCCESS;
//}
//
///*
// * Grant permissions to either a database or tin.
// */
//static ti_task_stat_e ti__task_props_del(
//        qp_map_t * task,
//        ti_event_t * event)
//{
//    ex_t * e = ex_use();
//    ti_db_t * db = event->target;
//    ti_thing_t * thing;
//    if (!event->target)
//        return ti__task_fail(event,
//            "props can only be deleted on things in a database");
//
//    qp_res_t * qid = qpx_map_get(task, TI_API_ID);
//    qp_res_t * props = qpx_map_get(task, TI_API_PROPS);
//
//    if (!qid || qid->tp != QP_RES_INT64)
//        return ti__task_fail(event,
//                "missing or invalid thing id ("TI_API_ID")");
//
//    if (!props || props->tp != QP_RES_ARRAY || !props->via.array->n)
//        return ti__task_fail(event,
//                "an array with at least one property is expected "
//                "("TI_API_PROPS")");
//
//    qp_array_t * arr_props = props->via.array;
//    for (size_t i = 0; i < arr_props->n; i++)
//        if (arr_props->values[i].tp != QP_RES_RAW)
//            return ti__task_fail(event,
//                    "each property should be a string "
//                    "("TI_API_PROPS")");
//
//    uint64_t id = (uint64_t) qid->via.int64;
//    thing = (ti_thing_t *) imap_get((qid->via.int64 < 0) ?
//            event->refthings : db->things, id);
//    if (!thing)
//    {
//        ex_set(e, -1, "cannot find thing: %"PRIu64, id);
//        return ti__task_fail(event, e->msg);
//    }
//
//    void * val;
//    ti_val_e tp;
//    qp_res_t * k, * v;
//    for (size_t i = 0; i < task->n; i++)
//    {
//        k = task->keys + i;
//        if (!*k->via.str || *k->via.str == TI_API_PREFIX[0]) continue;
//
//        ti_prop_t * prop = ti_db_props_get(db->props, k->via.str);
//        if (!prop) return TI_TASK_ERR;
//
//        v = task->values + i;
//        if (ti_thing_res_is_id(v))
//        {
//            int64_t sid = v->via.map->values[0].via.int64;
//            ti_thing_t * el = ti__task_thing_by_id(event, sid);
//            if (!el)
//            {
//                ex_set(e, -1, "cannot find thing: %"PRId64, sid);
//                return ti__task_fail(event, e->msg);
//            }
//            if (ti_thing_set(thing, prop, TI_VAL_ELEM, el))
//            {
//                log_critical(EX_ALLOC);
//                return TI_TASK_ERR;
//            }
//            continue;
//        }
//        switch (v->tp)
//        {
//        case QP_RES_MAP:
//            return ti__task_fail(event,
//                    "map must be an thing {\""TI_API_ID"\": <id>}");
//
//        case QP_RES_ARRAY:
//            assert(0);  /* TODO: fix arrays */
//            continue;
//        case QP_RES_INT64:
//            val = &v->via.int64;
//            tp = TI_VAL_INT;
//            break;
//        case QP_RES_REAL:
//            val = &v->via.real;
//            tp = TI_VAL_FLOAT;
//            break;
//        case QP_RES_RAW:
//            val = (void *) v->via.raw;
//            v->via.raw = NULL;
//            tp = TI_VAL_RAW;
//            break;
//        case QP_RES_BOOL:
//            val = &v->via.boolean;
//            tp = TI_VAL_BOOL;
//            break;
//        case QP_RES_NULL:
//            val = NULL;
//            tp = TI_VAL_NIL;
//            break;
//        case QP_RES_STR:
//            assert (0);
//            /* no break */
//        default:
//            assert (0);
//            break;
//        }
//
//        if (ti_thing_weak_set(thing, prop, tp, val))  // has_thing &&
//        {
//            log_critical(EX_ALLOC);
//            return TI_TASK_ERR;
//        }
//    }
//
//    if (qp_add_raw_from_str(event->result, TI_API_ID) ||
//        qp_add_int64(event->result, (int64_t) thing->id))
//    {
//        log_critical(EX_ALLOC);
//        return TI_TASK_ERR;
//    }
//
//    return TI_TASK_SUCCESS;
//}
//
//static inline ti_thing_t * ti__task_thing_by_id(
//        ti_event_t * event,
//        int64_t sid)
//{
//    return (ti_thing_t *) imap_get(
//            (sid < 0) ? event->refthings : event->target->things,
//            (uint64_t) sid);
//}
//
//static ti_thing_t * ti__task_thing_create(ti_event_t * event, int64_t sid)
//{
//    assert (sid < 0);
//    if (!event->refthings)
//    {
//        event->refthings = imap_create();
//        if (!event->refthings)
//            return NULL;
//    }
//    ti_thing_t * thing = ti_things_create(
//            event->target->things,
//            ti_get_id(event->events->tin));
//    /*
//     * we are allowed to overwrite the thing, the previous does hold a
//     * reference so it will be removed.
//     */
//    if (!thing || !imap_set(event->refthings, (uint64_t) sid, thing))
//        return NULL;
//
//    return thing;
//}
//
//static inline ti_task_stat_e ti__task_fail(
//        ti_event_t * event,
//        const char * msg)
//{
//    return ti__task_failn(event, msg, strlen(msg));
//}
//
//static ti_task_stat_e ti__task_failn(
//        ti_event_t * event,
//        const char * msg,
//        size_t n)
//{
//    if (event->client)
//    {
//        if (qp_add_raw(event->result, (const unsigned char *) "error_msg", 9) ||
//            qp_add_raw(event->result, (const unsigned char *) msg, n))
//        {
//            return TI_TASK_ERR;
//        }
//    }
//    return TI_TASK_FAILED;
//}
//


