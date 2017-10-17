/*
 * task.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <rql/task.h>
#include <util/ex.h>
#include <util/qpx.h>
#include <rql/api.h>

static rql_task_stat_e rql__task_create_user(
        qp_map_t * task,
        rql_event_t * event);
static rql_task_stat_e rql__task_fail(rql_event_t * event, const char * msg);
static rql_task_stat_e rql__task_failn(
        rql_event_t * event,
        const char * msg,
        size_t n);

rql_task_stat_e rql_task(
        qp_res_t * task,
        rql_event_t * event,
        rql_task_stat_e rc)
{
    qp_map_t * taskmap;
    qp_res_t * tasktp;
    rql_task_e tp;
    if (event->source && qp_add_map(&event->result))
    {
        rc = RQL_TASK_ERR;
        goto finish;
    }
    if (rc != RQL_TASK_SUCCESS)
    {
        rc = RQL_TASK_SKIPPED;
        goto finish;
    }
    if (task->tp != QP_RES_MAP)
    {
        rc = rql__task_fail(event, "expecting each task to be a map");
        goto finish;
    }
    taskmap = task->via.map;
    tasktp = qpx_map_get(taskmap, "_t");
    if (!tasktp ||
        tasktp->tp != QP_RES_INT64 ||
        tasktp->via.int64 < 0  ||
        tasktp->via.int64 > RQL_TASK_N)
    {
        rc = rql__task_fail(event, "invalid or missing task type (_t)");
        goto finish;
    }
    tp = (rql_task_e) tasktp->via.int64;
    switch (tp)
    {
    case RQL_TASK_CREATE_USER:
        rc = rql__task_create_user(taskmap, event);
        break;
    }

finish:
    if ((event->source && qp_add_raw(
            event->result,
            RQL_API_STAT,
            strlen(RQL_API_STAT))) ||
        qp_add_int64(event->result, rc) ||
        (event->source && qp_close_map(event->result))) return RQL_TASK_ERR;

    return rc;
}

static rql_task_stat_e rql__task_create_user(
        qp_map_t * task,
        rql_event_t * event)
{
    ex_ptr(e);
    qp_res_t * user, * pass;
    rql_user_t * usr;
    user = qpx_map_get(task, "_u");
    pass = qpx_map_get(task, "_p");
    if (!user || user->tp != QP_RES_STR ||
        !pass || pass->tp != QP_RES_STR)
    {
        return rql__task_fail(event,
                "missing or invalid user (_u) or password (_p)");
    }

    if (rql_user_name_check(user->via.str, e) ||
        rql_user_pass_check(pass->via.str, e))
    {
        return rql__task_failn(event, e->errmsg, e->n);
    }

    usr = rql_user_create(user->via.str, pass->via.str);
    if (!usr ||
        rql_user_set_pass(usr, usr->pass) ||
        vec_push(&event->rql->users, usr))
    {
        log_critical(EX_ALLOC);
        rql_user_drop(usr);
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
    if (event->source)
    {
        if (qp_add_raw(event->result, "error_msg", 9) ||
            qp_add_raw(event->result, msg, n)) return RQL_TASK_ERR;
    }
    return RQL_TASK_FAILED;
}
