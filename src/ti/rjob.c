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
static int rjob__new_collection(qp_unpacker_t * unp);
static int rjob__new_user(qp_unpacker_t * unp);
static int rjob__revoke(qp_unpacker_t * unp);


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

    if (qpx_obj_eq_str(&qp_job_name, "new_collection"))
        return rjob__new_collection(unp);

    if (qpx_obj_eq_str(&qp_job_name, "new_user"))
        return rjob__new_user(unp);

    if (qpx_obj_eq_str(&qp_job_name, "revoke"))
        return rjob__revoke(unp);

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
    ti_collection_t * target = NULL;
    uint64_t mask, user_id;
    qp_obj_t qp_target, qp_user, qp_mask;

    if (    !qp_is_map(qp_next(unp, NULL)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: target */
            !qp_is_int(qp_next(unp, &qp_target)) ||     /* val: target */
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: user */
            !qp_is_int(qp_next(unp, &qp_user)) ||       /* val: user */
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: mask */
            !qp_is_int(qp_next(unp, &qp_mask)))         /* val: mask */
    {
        log_critical("job `grant`: invalid format");
        return -1;
    }

    if (qp_target.via.int64)
    {
        uint64_t id = qp_target.via.int64;
        target = ti_collections_get_by_id(id);
        if (!target)
        {
            log_critical("job `grant`: "TI_COLLECTION_ID" not found", id);
            return -1;
        }
    }

    user_id = (uint64_t) qp_user.via.int64;

    user = ti_users_get_by_id(user_id);
    if (!user)
    {
        log_critical("job `grant`: "TI_USER_ID" not found", user_id);
        return -1;
    }

    mask = (uint64_t) qp_mask.via.int64;

    if (ti_access_grant(target ? &target->access : &ti()->access, user, mask))
    {
        log_critical(EX_ALLOC_S);
        return -1;
    }

    return 0;
}

/*
 * Returns 0 on success
 * - for example: {'name': collection_name, 'user': id, 'root': id}
 */
static int rjob__new_collection(qp_unpacker_t * unp)
{
    assert (unp);
    ex_t * e = ex_use();
    qp_obj_t qp_name, qp_user, qp_root;
    uint64_t user_id, root_id;
    ti_user_t * user;
    ti_collection_t * collection;

    if (    !qp_is_map(qp_next(unp, NULL)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: name */
            !qp_is_raw(qp_next(unp, &qp_name)) ||       /* val: name */
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: user */
            !qp_is_int(qp_next(unp, &qp_user)) ||       /* val: user */
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: root */
            !qp_is_int(qp_next(unp, &qp_root)))         /* val: root */
    {
        log_critical("job `new_collection`: invalid format");
        return -1;
    }

    user_id = (uint64_t) qp_user.via.int64;
    user = ti_users_get_by_id(user_id);
    if (!user)
    {
        log_critical("job `new_collection`: "TI_USER_ID" not found", user_id);
        return -1;
    }

    root_id = (uint64_t) qp_root.via.int64;

    collection = ti_collections_create_collection(
            root_id,
            (const char *) qp_name.via.raw,
            qp_name.len,
            user,
            e);
    if (!collection)
    {
        log_critical("job `new_collection`: %s", e->msg);
        return -1;
    }

    return 0;
}

/*
 * Returns 0 on success
 * - for example: {'id': id, 'username':value, 'password':value}
 */
static int rjob__new_user(qp_unpacker_t * unp)
{
    assert (unp);
    int rc = -1;
    ex_t * e = ex_use();
    qp_obj_t qp_id, qp_name, qp_pass;
    uint64_t user_id;
    char * encrypted;


    if (    !qp_is_map(qp_next(unp, NULL)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: id */
            !qp_is_int(qp_next(unp, &qp_id)) ||         /* val: id */
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: username */
            !qp_is_raw(qp_next(unp, &qp_name)) ||       /* val: username */
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: password */
            !qp_is_raw(qp_next(unp, &qp_pass)))         /* val: password */
    {
        log_critical("job `new_user`: invalid format");
        return -1;
    }

    user_id = (uint64_t) qp_id.via.int64;

    encrypted = qpx_obj_raw_to_str(&qp_pass);
    if (!encrypted)
    {
        log_critical(EX_ALLOC_S);
        return -1;
    }

    if (!ti_users_load_user(
            user_id,
            (const char *) qp_name.via.raw,
            qp_name.len,
            encrypted,
            e))
    {
        log_critical("job `new_user`: %s", e->msg);
        goto done;
    }

    rc = 0;

done:
    free(encrypted);
    return rc;
}

/*
 * Returns 0 on success
 * - for example: {'target':id, 'user':name, 'mask': integer}
 */
static int rjob__revoke(qp_unpacker_t * unp)
{
    assert (unp);

    ti_user_t * user;
    ti_collection_t * target = NULL;
    uint64_t mask, user_id;
    qp_obj_t qp_target, qp_user, qp_mask;

    if (    !qp_is_map(qp_next(unp, NULL)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: target */
            !qp_is_int(qp_next(unp, &qp_target)) ||     /* val: target */
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: user */
            !qp_is_int(qp_next(unp, &qp_user)) ||       /* val: user */
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: mask */
            !qp_is_int(qp_next(unp, &qp_mask)))         /* val: mask */
    {
        log_critical("job `revoke`: invalid format");
        return -1;
    }

    if (qp_target.via.int64)
    {
        uint64_t id = qp_target.via.int64;
        target = ti_collections_get_by_id(id);
        if (!target)
        {
            log_critical("job `revoke`: "TI_COLLECTION_ID" not found", id);
            return -1;
        }
    }

    user_id = (uint64_t) qp_user.via.int64;

    user = ti_users_get_by_id(user_id);
    if (!user)
    {
        log_critical("job `revoke`: "TI_USER_ID" not found", user_id);
        return -1;
    }

    mask = (uint64_t) qp_mask.via.int64;

    ti_access_revoke(target ? target->access : ti()->access, user, mask);

    return 0;
}

