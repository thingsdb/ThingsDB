/*
 * ti/rjob.c
 */
#include <assert.h>
#include <ti/rjob.h>
#include <ti/val.h>
#include <ti.h>
#include <ti/users.h>
#include <ti/access.h>
#include <util/qpx.h>

static int rjob__grant(qp_unpacker_t * unp);
static int rjob__user_new(qp_unpacker_t * unp);


int ti_rjob_run(qp_unpacker_t * unp)
{
    qp_obj_t qp_job_name;
    if (!qp_is_raw(qp_next(unp, &qp_job_name)) || qp_job_name.len < 2)
    {
        log_critical("job `type` for thing "TI_THING_ID" is missing", 0);
        return -1;
    }

    if (qpx_obj_eq_str(&qp_job_name, "grant"))
        return rjob__grant(unp);

    if (qpx_obj_eq_str(&qp_job_name, "user_new"))
        return rjob__user_new(unp);

    log_critical("unknown job: `%.*s`",
            (int) qp_job_name.len,
            (char *) qp_job_name.via.raw);
    return -1;
}

/*
 * Returns 0 on success
 * - for example: {'target':id, 'user':name, 'mask': integer}
 */
static int rjob__grant(qp_unpacker_t * unp)
{
    assert (unp);

    ti_user_t * user;
    ti_db_t * target = NULL;
    uint64_t mask;
    qp_obj_t qp_target, qp_user, qp_mask;

    if (    !qp_is_map(qp_next(unp, NULL)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: target */
            !qp_is_int(qp_next(unp, &qp_target)) ||     /* val: target */
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: user */
            !qp_is_raw(qp_next(unp, &qp_user)) ||       /* val: user */
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: mask */
            !qp_is_int(qp_next(unp, &qp_mask)))         /* val: mask */
    {
        log_critical("job `grant`: invalid format");
        return -1;
    }

    if (qp_target.via.int64)
    {
        uint64_t id = qp_target.via.int64;
        target = ti_dbs_get_by_id(id);
        if (!target)
        {
            log_critical("job `grant`: "TI_DB_ID" not found", id);
            return -1;
        }
    }

    user = ti_users_get_by_namestrn(
            (const char *) qp_user.via.raw,
            qp_user.len);
    if (!user)
    {
        log_critical("job `grant`: user `%.*s` not found",
                (int) qp_user.len, (char *) qp_user.via.raw);
        return -1;
    }

    mask = qp_mask.via.int64;

    if (ti_access_grant(target ? &target->access : &ti()->access, user, mask))
    {
        log_critical(EX_ALLOC_S);
        return -1;
    }

    return 0;
}

/*
 * Returns 0 on success
 * - for example: {'username':value, 'password':value}
 */
static int rjob__user_new(qp_unpacker_t * unp)
{
    assert (unp);
    int rc = -1;
    ex_t * e = ex_use();
    qp_obj_t qp_name, qp_pass;
    char * passstr;

    if (    !qp_is_map(qp_next(unp, NULL)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: username */
            !qp_is_raw(qp_next(unp, &qp_name)) ||       /* val: username */
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: password */
            !qp_is_raw(qp_next(unp, &qp_pass)))         /* val: password */
    {
        log_critical("job `user_new`: invalid format");
        return -1;
    }

    passstr = qpx_obj_raw_to_str(&qp_pass);
    if (!passstr)
    {
        log_critical(EX_ALLOC_S);
        return -1;
    }

    if (!ti_users_create_user(
            (const char *) qp_name.via.raw, qp_name.len,
            passstr, e))
    {
        log_critical("job `user_new`: %s", e->msg);
        goto done;
    }

    rc = 0;

done:
    free(passstr);
    return rc;
}
