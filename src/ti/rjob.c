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
#include <util/cryptx.h>

static int rjob__del_collection(qp_unpacker_t * unp);
static int rjob__del_user(qp_unpacker_t * unp);
static int rjob__grant(qp_unpacker_t * unp);
static int rjob__new_collection(qp_unpacker_t * unp);
static int rjob__new_node(qp_unpacker_t * unp);
static int rjob__new_user(qp_unpacker_t * unp);
static int rjob__revoke(qp_unpacker_t * unp);
static int rjob__set_password(qp_unpacker_t * unp);
static int rjob__set_quota(qp_unpacker_t * unp);


int ti_rjob_run(qp_unpacker_t * unp)
{
    qp_obj_t qp_job_name;
    if (!qp_is_raw(qp_next(unp, &qp_job_name)) || qp_job_name.len < 2)
    {
        log_critical("job `type` for thing "TI_THING_ID" is missing", 0);
        return -1;
    }

    switch (*qp_job_name.via.raw)
    {
    case 'd':
        if (qpx_obj_eq_str(&qp_job_name, "del_collection"))
            return rjob__del_collection(unp);
        if (qpx_obj_eq_str(&qp_job_name, "del_user"))
            return rjob__del_user(unp);
        break;
    case 'g':
        if (qpx_obj_eq_str(&qp_job_name, "grant"))
            return rjob__grant(unp);
        break;
    case 'n':
        if (qpx_obj_eq_str(&qp_job_name, "new_collection"))
            return rjob__new_collection(unp);
        if (qpx_obj_eq_str(&qp_job_name, "new_node"))
            return rjob__new_node(unp);
        if (qpx_obj_eq_str(&qp_job_name, "new_user"))
            return rjob__new_user(unp);
        break;
    case 'r':
        if (qpx_obj_eq_str(&qp_job_name, "revoke"))
            return rjob__revoke(unp);
        break;
    case 's':
        if (qpx_obj_eq_str(&qp_job_name, "set_password"))
            return rjob__set_password(unp);
        if (qpx_obj_eq_str(&qp_job_name, "set_quota"))
            return rjob__set_quota(unp);
        break;
    }

    log_critical("unknown job: `%.*s`",
            (int) qp_job_name.len,
            (char *) qp_job_name.via.raw);
    return -1;
}

/*
 * Returns 0 on success
 * - for example: id
 */
static int rjob__del_collection(qp_unpacker_t * unp)
{
    assert (unp);

    uint64_t collection_id;
    qp_obj_t qp_collection;

    if (!qp_is_int(qp_next(unp, &qp_collection)))
    {
        log_critical("job `del_collection`: invalid format");
        return -1;
    }

    collection_id = (uint64_t) qp_collection.via.int64;

    if (!ti_collections_del_collection(collection_id))
    {
        log_critical(
                "job `del_collection`: "TI_COLLECTION_ID" not found",
                collection_id);
        return -1;
    }

    return 0;
}

/*
 * Returns 0 on success
 * - for example: id
 */
static int rjob__del_user(qp_unpacker_t * unp)
{
    assert (unp);

    ti_user_t * user;
    uint64_t user_id;
    qp_obj_t qp_user;

    if (!qp_is_int(qp_next(unp, &qp_user)))
    {
        log_critical("job `del_user`: invalid format");
        return -1;
    }

    user_id = (uint64_t) qp_user.via.int64;
    user = ti_users_get_by_id(user_id);
    if (!user)
    {
        log_critical("job `del_user`: "TI_USER_ID" not found", user_id);
        return -1;
    }

    ti_users_del_user(user);
    return 0;
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
 * - for example: {
 *      'id': id,
 *      'zone': zone,
 *      'port': port,
 *      'addr':ip_addr,
 *      'secret': encrypted
 *   }
 */
static int rjob__new_node(qp_unpacker_t * unp)
{
    assert (unp);
    qp_obj_t qp_id, qp_zone, qp_port, qp_addr, qp_secret;
    uint8_t node_id, zone, next_node_id = ti()->nodes->vec->n;
    uint16_t port;
    char addr[INET6_ADDRSTRLEN];

    assert (((ti_node_t *) vec_last(ti()->nodes->vec))->id == next_node_id-1);

    if (    !qp_is_map(qp_next(unp, NULL)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||
            !qp_is_int(qp_next(unp, &qp_id)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||
            !qp_is_int(qp_next(unp, &qp_zone)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||
            !qp_is_int(qp_next(unp, &qp_port)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||
            !qp_is_raw(qp_next(unp, &qp_addr)) ||
            qp_addr.len >= INET6_ADDRSTRLEN ||
            !qp_is_raw(qp_next(unp, NULL)) ||
            !qp_is_raw(qp_next(unp, &qp_secret)) ||
            qp_secret.len != CRYPTX_SZ ||
            qp_secret.via.raw[qp_secret.len-1] != '\0')
    {
        log_critical("job `new_node`: invalid format");
        return -1;
    }

    node_id = (uint8_t) qp_id.via.int64;
    zone = (uint8_t) qp_zone.via.int64;
    port = (uint16_t) qp_port.via.int64;

    if (node_id < next_node_id)
        return 0;  /* this node is already added */

    if (node_id > next_node_id)
    {
        log_critical(
                "job `new_node`: expecting "TI_NODE_ID" but got "TI_NODE_ID,
                next_node_id, node_id);
        return -1;
    }

    memcpy(addr, qp_addr.via.raw, qp_addr.len);
    addr[qp_addr.len] = '\0';

    if (!ti_nodes_new_node(zone, port, addr, (const char *) qp_secret.via.raw))
    {
        log_critical(EX_ALLOC_S);
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

/*
 * Returns 0 on success
 * - for example: {'id':user_id, 'password': encpass}
 */
static int rjob__set_password(qp_unpacker_t * unp)
{
    assert (unp);
    qp_obj_t qp_user, qp_pass;
    uint64_t user_id;
    ti_user_t * user;
    char * encrypted;

    if (    !qp_is_map(qp_next(unp, NULL)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: id */
            !qp_is_int(qp_next(unp, &qp_user)) ||       /* val: id */
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: password */
            !qp_is_raw(qp_next(unp, &qp_pass)))         /* val: password */
    {
        log_critical("job `set_password`: invalid format");
        return -1;
    }

    user_id = (uint64_t) qp_user.via.int64;
    user = ti_users_get_by_id(user_id);
    if (!user)
    {
        log_critical("job `set_password`: "TI_USER_ID" not found", user_id);
        return -1;
    }

    encrypted = qpx_obj_raw_to_str(&qp_pass);
    if (!encrypted)
    {
        log_critical(EX_ALLOC_S);
        return -1;
    }

    free(user->encpass);
    user->encpass = encrypted;

    return 0;
}

/*
 * Returns 0 on success
 * - for example: {'collection':id, 'quota_tp': quota_enum_t, 'quota': size_t}
 */
static int rjob__set_quota(qp_unpacker_t * unp)
{
    assert (unp);

    size_t quota;
    ti_collection_t * collection;
    ti_quota_enum_t quota_tp;
    qp_obj_t qp_collection, qp_quota_tp, qp_quota;

    if (    !qp_is_map(qp_next(unp, NULL)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||
            !qp_is_int(qp_next(unp, &qp_collection)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||
            !qp_is_int(qp_next(unp, &qp_quota_tp)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||
            !qp_is_int(qp_next(unp, &qp_quota)))
    {
        log_critical("job `set_quota`: invalid format");
        return -1;
    }

    if (qp_collection.via.int64)
    {
        uint64_t id = qp_collection.via.int64;
        collection = ti_collections_get_by_id(id);
        if (!collection)
        {
            log_critical("job `set_quota`: "TI_COLLECTION_ID" not found", id);
            return -1;
        }
    }

    quota_tp = (ti_quota_enum_t) qp_quota_tp.via.int64;
    quota = (size_t) qp_quota.via.int64;
    ti_collection_set_quota(collection, quota_tp, quota);

    return 0;
}

