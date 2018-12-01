/*
 * ti/rjob.c
 */
#include <assert.h>
#include <ti/rjob.h>
#include <ti/val.h>
#include <ti/name.h>
#include <ti/names.h>

static int rjob__user_new(qp_unpacker_t * unp);


int ti_rjob_run(qp_unpacker_t * unp)
{
    qp_obj_t qp_job_name;
    const uchar * raw;
    if (!qp_is_raw(qp_next(unp, &qp_job_name)) || qp_job_name.len < 2)
    {
        log_critical("job `type` for thing "TI_THING_ID" is missing", 0);
        return -1;
    }

    if (qpx_obj_eq_str(qp_job_name, "user_new"))
        return rjob__user_new(unp);

    log_critical("unknown job: `%.*s`", (int) qp_job_name.len, (char *) raw);
    return -1;
}

/*
 * Returns 0 on success
 * - for example: {'username':value, 'password':value}
 */
static int rjob__user_new(qp_unpacker_t * unp)
{
    assert (unp);

    ex_t * e = ex_use();
    ti_val_t val;
    ti_name_t * name;
    qp_obj_t qp_name, qp_pass;

    if (    !qp_is_map(qp_next(unp, NULL)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: username */
            !qp_is_raw(qp_next(unp, &qp_name)) ||       /* val: username */
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: password */
            !qp_is_raw(qp_next(unp, &qp_pass)))         /* val: password */
    {
        log_critical("job `user_new`: invalid format", 0);
        return -1;
    }

    char * passstr = qpx_obj_raw_to_str(&qp_pass);
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
        return -1;
    }

    return 0;
}
