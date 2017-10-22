/*
 * task.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <assert.h>
#include <rql/task.h>
#include <util/qpx.h>
#include <rql/api.h>
#include <rql/proto.h>
#include <rql/access.h>
#include <rql/users.h>
#include <rql/dbs.h>
#include <rql/auth.h>

static rql_task_stat_e rql__task_user_create(
        qp_map_t * task,
        rql_event_t * event);
static rql_task_stat_e rql__task_db_create(
        qp_map_t * task,
        rql_event_t * event);
static rql_task_stat_e rql__task_grant(
        qp_map_t * task,
        rql_event_t * event);
static rql_task_stat_e rql__task_fail(rql_event_t * event, const char * msg);
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
    default:
        assert (0);
    }

finish:
    if ((event->client && qp_add_raw(
            event->result,
            RQL_API_STAT,
            strlen(RQL_API_STAT))) ||
        qp_add_int64(event->result, rc) ||
        (event->client && qp_close_map(event->result))) return RQL_TASK_ERR;

    return rc;
}

const char * rql_task_str(rql_task_t * task)
{
    switch(task->tp)
    {
    case RQL_TASK_USER_CREATE: return "TASK_USER_CREATE";
    case RQL_TASK_GRANT: return "TASK_GRANT";
    default:
        return "TASK_UNKNOWN";
    }
}

static rql_task_stat_e rql__task_user_create(
        qp_map_t * task,
        rql_event_t * event)
{
    ex_ptr(e);
    qp_res_t * user, * pass;
    rql_user_t * usr;
    user = qpx_map_get(task, RQL_API_USER);
    pass = qpx_map_get(task, RQL_API_PASS);
    if (!user || user->tp != QP_RES_STR ||
        !pass || pass->tp != QP_RES_STR)
    {
        return rql__task_fail(event,
                "missing or invalid user ("RQL_API_USER") "
                "or password ("RQL_API_PASS")");
    }

    if (rql_user_name_check(user->via.str, e) ||
        rql_user_pass_check(pass->via.str, e))
    {
        return rql__task_failn(event, e->errmsg, e->n);
    }

    if (rql_users_get_by_name(event->events->rql->users, user->via.str))
    {
        return rql__task_fail(event, "user already exists");
    }

    usr = rql_user_create(
            rql_events_get_obj_id(event->events),
            user->via.str,
            pass->via.str);
    if (!usr ||
        rql_user_set_pass(usr, usr->pass) ||
        vec_push(&event->events->rql->users, usr))
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
    ex_ptr(e);
    qp_res_t * quser, * qname;
    rql_user_t * user;
    rql_db_t * db;
    quser = qpx_map_get(task, RQL_API_USER);
    qname = qpx_map_get(task, RQL_API_NAME);
    if (!quser || quser->tp != QP_RES_STR ||
        !qname || qname->tp != QP_RES_STR)
    {
        return rql__task_fail(event,
                "missing or invalid user ("RQL_API_USER") "
                "or database name ("RQL_API_NAME")");
    }

    if (rql_db_name_check(qname->via.str, e))
    {
        return rql__task_failn(event, e->errmsg, e->n);
    }

    if (rql_dbs_get_by_name(event->events->rql->dbs, qname->via.str))
    {
        return rql__task_fail(event, "database already exists");
    }

    user = rql_users_get_by_name(event->events->rql->users, quser->via.str);
    if (!user) return rql__task_fail(event, "user not found");

    guid_t guid;
    guid_init(&guid, rql_events_get_obj_id(event->events));

    db = rql_db_create(&guid, qname->via.str);
    if (!db || vec_push(&event->events->rql->dbs, db))
    {
        log_critical(EX_ALLOC);
        rql_db_drop(db);
        return RQL_TASK_ERR;
    }

    if (rql_access_grant(db->access, user, RQL_AUTH_MASK_FULL))
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

    if (!quser || quser->tp != QP_RES_STR ||
        !qperm || qperm->tp != QP_RES_INT64)
    {
        return rql__task_fail(event,
                "missing or invalid user ("RQL_API_USER") "
                "or permission flags ("RQL_API_PERM")");
    }

    rql_user_t * user = rql_users_get_by_name(
            event->events->rql->users,
            quser->via.str);

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

static rql_task_stat_e rql__task_fail(rql_event_t * event, const char * msg)
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
        if (qp_add_raw(event->result, "error_msg", 9) ||
            qp_add_raw(event->result, msg, n)) return RQL_TASK_ERR;
    }
    return RQL_TASK_FAILED;
}
