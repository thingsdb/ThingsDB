/*
 * task.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <assert.h>
#include <rql/access.h>
#include <rql/api.h>
#include <rql/auth.h>
#include <rql/dbs.h>
#include <rql/elem.h>
#include <rql/elems.h>
#include <rql/inliners.h>
#include <rql/prop.h>
#include <rql/props.h>
#include <rql/proto.h>
#include <rql/task.h>
#include <rql/users.h>
#include <util/qpx.h>

static rql_task_stat_e rql__task_user_create(
        qp_map_t * task,
        rql_event_t * event);
static rql_task_stat_e rql__task_db_create(
        qp_map_t * task,
        rql_event_t * event);
static rql_task_stat_e rql__task_grant(
        qp_map_t * task,
        rql_event_t * event);
static rql_task_stat_e rql__task_props_set(
        qp_map_t * task,
        rql_event_t * event);
static rql_task_stat_e rql__task_props_del(
        qp_map_t * task,
        rql_event_t * event);
static inline rql_elem_t * rql__task_elem_by_id(
        rql_event_t * event,
        int64_t sid);
static rql_elem_t * rql__task_elem_create(rql_event_t * event, int64_t sid);
static inline rql_task_stat_e rql__task_fail(
        rql_event_t * event,
        const char * msg);
static rql_task_stat_e rql__task_failn(
        rql_event_t * event,
        const char * msg,
        size_t n);

rql_task_t * rql_task_create(qp_res_t * res, ex_t * e)
{
    rql_task_t * task;
    qp_res_t * tasktp;
    if (res->tp != QP_RES_MAP)
    {
        ex_set(e, RQL_PROTO_TYPE_ERR, "expecting each task to be a map");
        return NULL;
    }
    tasktp = qpx_map_get(res->via.map, RQL_API_TASK);
    if (!tasktp ||
        tasktp->tp != QP_RES_INT64 ||
        tasktp->via.int64 < 0  ||
        tasktp->via.int64 > RQL_TASK_N)
    {
        ex_set(e, RQL_PROTO_TYPE_ERR,
                "invalid or missing task type ("RQL_API_TASK")");
        return NULL;
    }

    task = (rql_task_t *) malloc(sizeof(rql_task_t));
    if (!task)
    {
        ex_set_alloc(e);
        return NULL;
    }

    task->tp = (rql_task_e) tasktp->via.int64;
    task->res = res;

    return task;
}

void rql_task_destroy(rql_task_t * task)
{
    if (!task) return;
    if (task->res)
    {
        qp_res_destroy(task->res);
    }
    free(task);
}

rql_task_stat_e rql_task_run(
        rql_task_t * task,
        rql_event_t * event,
        rql_task_stat_e rc)
{
    if (event->client && qp_add_map(&event->result))
    {
        rc = RQL_TASK_ERR;
        goto finish;
    }
    if (rc != RQL_TASK_SUCCESS)
    {
        rc = RQL_TASK_SKIPPED;
        goto finish;
    }
    qp_map_t * map = task->res->via.map;
    switch (task->tp)
    {
    case RQL_TASK_USER_CREATE:
        rc = rql__task_user_create(map, event);
        break;
    case RQL_TASK_DB_CREATE:
        rc = rql__task_db_create(map, event);
        break;
    case RQL_TASK_GRANT:
        rc = rql__task_grant(map, event);
        break;
    case RQL_TASK_PROPS_SET:
        rc = rql__task_props_set(map, event);
        break;
    case RQL_TASK_PROPS_DEL:
        rc = rql__task_props_del(map, event);
        break;
    default:
        assert (0);
    }

finish:
    if ((event->client && qp_add_raw_from_str(event->result, RQL_API_STAT)) ||
        qp_add_int64(event->result, rc) ||
        (event->client && qp_close_map(event->result))) return RQL_TASK_ERR;

    return rc;
}

const char * rql_task_str(rql_task_t * task)
{
    switch(task->tp)
    {
    case RQL_TASK_USER_CREATE: return "TASK_USER_CREATE";
    case RQL_TASK_USER_DROP: return "TASK_USER_DROP";
    case RQL_TASK_USER_ALTER: return "TASK_USER_ALTER";
    case RQL_TASK_DB_CREATE: return "TASK_DB_CREATE";
    case RQL_TASK_DB_RENAME: return "TASK_DB_RENAME";
    case RQL_TASK_DB_DROP: return "TASK_DB_DROP";
    case RQL_TASK_GRANT: return "TASK_GRANT";
    case RQL_TASK_REVOKE: return "TASK_REVOKE";
    case RQL_TASK_SET_REDUNDANCY: return "TASK_SET_REDUNDANCY";
    case RQL_TASK_NODE_ADD: return "TASK_NODE_ADD";
    case RQL_TASK_NODE_REPLACE: return "TASK_NODE_REPLACE";
    case RQL_TASK_SUBSCRIBE: return "TASK_SUBSCRIBE";
    case RQL_TASK_UNSUBSCRIBE: return "TASK_UNSUBSCRIBE";
    case RQL_TASK_PROPS_SET: return "TASK_PROPS_SET";
    case RQL_TASK_PROPS_DEL: return "TASK_PROPS_DEL";
    default:
        return "TASK_UNKNOWN";
    }
}

static rql_task_stat_e rql__task_user_create(
        qp_map_t * task,
        rql_event_t * event)
{
    ex_t * e = ex_use();
    if (event->target)
    {
        ex_set(e, -1, "users can only be created on rql: '%s'", rql_name);
        return rql__task_fail(event, e->msg);
    }
    rql_t * rql = event->events->rql;
    qp_res_t * user, * pass;
    rql_user_t * usr;
    user = qpx_map_get(task, RQL_API_USER);
    pass = qpx_map_get(task, RQL_API_PASS);
    if (!user || user->tp != QP_RES_RAW ||
        !pass || pass->tp != QP_RES_RAW)
    {
        return rql__task_fail(event,
                "missing or invalid user ("RQL_API_USER") "
                "or password ("RQL_API_PASS")");
    }

    if (rql_user_name_check(user->via.raw, e) ||
        rql_user_pass_check(pass->via.raw, e))
    {
        return rql__task_fail(event, e->msg);
    }

    if (rql_users_get_by_name(rql->users, user->via.raw))
    {
        return rql__task_fail(event, "user already exists");
    }

    char * passstr = rql_raw_to_str(pass->via.raw);
    if (!passstr)
    {
        log_critical(EX_ALLOC);
        return RQL_TASK_ERR;
    }

    usr = rql_user_create(
            rql_get_id(rql),
            user->via.raw,
            passstr);

    free(passstr);

    if (!usr ||
        rql_user_set_pass(usr, usr->pass) ||
        vec_push(&rql->users, usr))
    {
        log_critical(EX_ALLOC);
        rql_user_drop(usr);
        return RQL_TASK_ERR;
    }

    return RQL_TASK_SUCCESS;
}

static rql_task_stat_e rql__task_db_create(
        qp_map_t * task,
        rql_event_t * event)
{
    ex_t * e = ex_use();
    if (event->target)
    {
        ex_set(e, -1, "databases can only be created on rql: '%s'", rql_name);
        return rql__task_fail(event, e->msg);
    }
    rql_t * rql = event->events->rql;
    qp_res_t * quser, * qname;
    rql_user_t * user;
    rql_db_t * db;
    quser = qpx_map_get(task, RQL_API_USER);
    qname = qpx_map_get(task, RQL_API_NAME);
    if (!quser || quser->tp != QP_RES_RAW ||
        !qname || qname->tp != QP_RES_RAW)
    {
        return rql__task_fail(event,
                "missing or invalid user ("RQL_API_USER") "
                "or database name ("RQL_API_NAME")");
    }

    if (rql_db_name_check(qname->via.raw, e))
    {
        return rql__task_fail(event, e->msg);
    }

    if (rql_dbs_get_by_name(rql->dbs, qname->via.raw))
    {
        return rql__task_fail(event, "database already exists");
    }

    user = rql_users_get_by_name(rql->users, quser->via.raw);
    if (!user) return rql__task_fail(event, "user not found");

    guid_t guid;
    guid_init(&guid, rql_get_id(rql));

    db = rql_db_create(rql, &guid, qname->via.raw);
    if (!db || vec_push(&rql->dbs, db))
    {
        log_critical(EX_ALLOC);
        rql_db_drop(db);
        return RQL_TASK_ERR;
    }

    if (rql_access_grant(&db->access, user, RQL_AUTH_MASK_FULL) ||
        rql_db_buid(db) ||
        qp_add_raw_from_str(event->result, RQL_API_ID) ||
        qp_add_int64(event->result, (int64_t) db->root->id))
    {
        log_critical(EX_ALLOC);
        return RQL_TASK_ERR;
    }

    return RQL_TASK_SUCCESS;
}

/*
 * Grant permissions to either a database or rql.
 */
static rql_task_stat_e rql__task_grant(
        qp_map_t * task,
        rql_event_t * event)
{
    qp_res_t * quser = qpx_map_get(task, RQL_API_USER);
    qp_res_t * qperm = qpx_map_get(task, RQL_API_PERM);

    if (!quser || quser->tp != QP_RES_RAW ||
        !qperm || qperm->tp != QP_RES_INT64)
    {
        return rql__task_fail(event,
                "missing or invalid user ("RQL_API_USER") "
                "or permission flags ("RQL_API_PERM")");
    }

    rql_user_t * user = rql_users_get_by_name(
            event->events->rql->users,
            quser->via.raw);

    if (!user) return rql__task_fail(event, "user not found");

    if (rql_access_grant(
            (event->target) ?
                    &event->target->access : &event->events->rql->access,
            user,
            (uint64_t) qperm->via.int64))
    {
        log_critical(EX_ALLOC);
        return RQL_TASK_ERR;
    }

    return RQL_TASK_SUCCESS;
}


/*
 * Grant permissions to either a database or rql.
 */
static rql_task_stat_e rql__task_props_set(
        qp_map_t * task,
        rql_event_t * event)
{
    ex_t * e = ex_use();
    rql_t * rql = event->events->rql;
    rql_db_t * db = event->target;
    rql_elem_t * elem;
    if (!event->target)
    {
        return rql__task_fail(event,
            "props can only be set on elements in a database");
    }
    qp_res_t * qid = qpx_map_get(task, RQL_API_ID);

    if (!qid || qid->tp != QP_RES_INT64)
    {
        return rql__task_fail(event,
                "missing or invalid element id ("RQL_API_ID")");
    }

    uint64_t id = (uint64_t) qid->via.int64;
    if (qid->via.int64 < 0)
    {
        elem = rql__task_elem_create(event, id);
        if (!elem) return RQL_TASK_ERR;
    }
    else
    {
        elem = (rql_elem_t *) imap_get(db->elems, id);
        if (!elem)
        {
            ex_set(e, -1, "cannot find element: %"PRIu64, id);
            return rql__task_fail(event, e->msg);
        }
    }

    _Bool has_elem = rql_has_id(rql, elem->id);

    void * val;
    rql_val_e tp;

    qp_res_t * k, * v;
    for (size_t i = 0; i < task->n; i++)
    {
        k = task->keys + i;
        if (!*k->via.str || *k->via.str == RQL_API_PREFIX[0]) continue;

        rql_prop_t * prop = rql_db_props_get(db->props, k->via.str);
        if (!prop) return RQL_TASK_ERR;

        v = task->values + i;
        if (rql_elem_res_is_id(v))
        {
            int64_t sid = v->via.map->values[0].via.int64;
            rql_elem_t * el = rql__task_elem_by_id(event, sid);
            if (!el)
            {
                ex_set(e, -1, "cannot find element: %"PRId64, sid);
                return rql__task_fail(event, e->msg);
            }
            if (rql_elem_set(elem, prop, RQL_VAL_ELEM, el))
            {
                log_critical(EX_ALLOC);
                return RQL_TASK_ERR;
            }
            continue;
        }
        switch (v->tp)
        {
        case QP_RES_MAP:
            return rql__task_fail(event,
                    "map must be an element {\""RQL_API_ID"\": <id>}");

        case QP_RES_ARRAY:
            assert(0);  /* TODO: fix arrays */
            continue;
        case QP_RES_INT64:
            val = &v->via.int64;
            tp = RQL_VAL_INT;
            break;
        case QP_RES_REAL:
            val = &v->via.real;
            tp = RQL_VAL_FLOAT;
            break;
        case QP_RES_RAW:
            val = (void *) v->via.raw;
            v->via.raw = NULL;
            tp = RQL_VAL_RAW;
            break;
        case QP_RES_BOOL:
            val = &v->via.boolean;
            tp = RQL_VAL_BOOL;
            break;
        case QP_RES_NULL:
            val = NULL;
            tp = RQL_VAL_NIL;
            break;
        case QP_RES_STR:
            assert (0);
            /* no break */
        default:
            assert (0);
            break;
        }
        if (has_elem && rql_elem_weak_set(elem, prop, tp, val))
        {
            log_critical(EX_ALLOC);
            return RQL_TASK_ERR;
        }
    }

    if (qp_add_raw_from_str(event->result, RQL_API_ID) ||
        qp_add_int64(event->result, (int64_t) elem->id))
    {
        log_critical(EX_ALLOC);
        return RQL_TASK_ERR;
    }

    return RQL_TASK_SUCCESS;
}

/*
 * Grant permissions to either a database or rql.
 */
static rql_task_stat_e rql__task_props_del(
        qp_map_t * task,
        rql_event_t * event)
{
    ex_t * e = ex_use();
    rql_t * rql = event->events->rql;
    rql_db_t * db = event->target;
    rql_elem_t * elem;
    if (!event->target)
        return rql__task_fail(event,
            "props can only be deleted on elements in a database");

    qp_res_t * qid = qpx_map_get(task, RQL_API_ID);
    qp_res_t * props = qpx_map_get(task, RQL_API_PROPS);

    if (!qid || qid->tp != QP_RES_INT64)
        return rql__task_fail(event,
                "missing or invalid element id ("RQL_API_ID")");

    if (!props || !props->tp == QP_RES_ARRAY || !props->via.array->n)
        return rql__task_fail(event,
                "an array with at least one property is expected "
                "("RQL_API_PROPS")");

    qp_array_t * arr_props = props->via.array;
    for (size_t i = 0; i < arr_props->n; i++)
        if (arr_props->values[i].tp != QP_RES_RAW)
            return rql__task_fail(event,
                    "each property should be a string "
                    "("RQL_API_PROPS")");

    uint64_t id = (uint64_t) qid->via.int64;
    elem = (rql_elem_t *) imap_get((qid->via.int64 < 0) ?
            event->refelems : db->elems, id);
    if (!elem)
    {
        ex_set(e, -1, "cannot find element: %"PRIu64, id);
        return rql__task_fail(event, e->msg);
    }

    void * val;
    rql_val_e tp;
    qp_res_t * k, * v;
    for (size_t i = 0; i < task->n; i++)
    {
        k = task->keys + i;
        if (!*k->via.str || *k->via.str == RQL_API_PREFIX[0]) continue;

        rql_prop_t * prop = rql_db_props_get(db->props, k->via.str);
        if (!prop) return RQL_TASK_ERR;

        v = task->values + i;
        if (rql_elem_res_is_id(v))
        {
            int64_t sid = v->via.map->values[0].via.int64;
            rql_elem_t * el = rql__task_elem_by_id(event, sid);
            if (!el)
            {
                ex_set(e, -1, "cannot find element: %"PRId64, sid);
                return rql__task_fail(event, e->msg);
            }
            if (rql_elem_set(elem, prop, RQL_VAL_ELEM, el))
            {
                log_critical(EX_ALLOC);
                return RQL_TASK_ERR;
            }
            continue;
        }
        switch (v->tp)
        {
        case QP_RES_MAP:
            return rql__task_fail(event,
                    "map must be an element {\""RQL_API_ID"\": <id>}");

        case QP_RES_ARRAY:
            assert(0);  /* TODO: fix arrays */
            continue;
        case QP_RES_INT64:
            val = &v->via.int64;
            tp = RQL_VAL_INT;
            break;
        case QP_RES_REAL:
            val = &v->via.real;
            tp = RQL_VAL_FLOAT;
            break;
        case QP_RES_RAW:
            val = (void *) v->via.raw;
            v->via.raw = NULL;
            tp = RQL_VAL_RAW;
            break;
        case QP_RES_BOOL:
            val = &v->via.boolean;
            tp = RQL_VAL_BOOL;
            break;
        case QP_RES_NULL:
            val = NULL;
            tp = RQL_VAL_NIL;
            break;
        case QP_RES_STR:
            assert (0);
            /* no break */
        default:
            assert (0);
            break;
        }

        if (has_elem && rql_elem_weak_set(elem, prop, tp, val))
        {
            log_critical(EX_ALLOC);
            return RQL_TASK_ERR;
        }
    }

    if (qp_add_raw_from_str(event->result, RQL_API_ID) ||
        qp_add_int64(event->result, (int64_t) elem->id))
    {
        log_critical(EX_ALLOC);
        return RQL_TASK_ERR;
    }

    return RQL_TASK_SUCCESS;
}

static inline rql_elem_t * rql__task_elem_by_id(
        rql_event_t * event,
        int64_t sid)
{
    return (rql_elem_t *) imap_get(
            (sid < 0) ? event->refelems : event->target->elems,
            (uint64_t) sid);
}

static rql_elem_t * rql__task_elem_create(rql_event_t * event, int64_t sid)
{
    assert (sid < 0);
    if (!event->refelems)
    {
        event->refelems = imap_create();
        if (!event->refelems)
            return NULL;
    }
    rql_elem_t * elem = rql_elems_create(
            event->target->elems,
            rql_get_id(event->events->rql));
    /*
     * we are allowed to overwrite the element, the previous does hold a
     * reference so it will be removed.
     */
    if (!elem || !imap_set(event->refelems, (uint64_t) sid, elem))
        return NULL;

    return elem;
}

static inline rql_task_stat_e rql__task_fail(
        rql_event_t * event,
        const char * msg)
{
    return rql__task_failn(event, msg, strlen(msg));
}

static rql_task_stat_e rql__task_failn(
        rql_event_t * event,
        const char * msg,
        size_t n)
{
    if (event->client)
    {
        if (qp_add_raw(event->result, (const unsigned char *) "error_msg", 9) ||
            qp_add_raw(event->result, (const unsigned char *) msg, n))
        {
            return RQL_TASK_ERR;
        }
    }
    return RQL_TASK_FAILED;
}



