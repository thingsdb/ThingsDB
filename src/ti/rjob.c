/*
 * ti/rjob.c
 */
#include <assert.h>
#include <ti.h>
#include <ti/access.h>
#include <ti/procedure.h>
#include <ti/procedures.h>
#include <ti/rjob.h>
#include <ti/syntax.h>
#include <ti/users.h>
#include <ti/val.h>
#include <util/cryptx.h>
#include <util/qpx.h>

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
 * - for example: after_is
 */
static int rjob__del_expired(qp_unpacker_t * unp)
{
    assert (unp);

    uint64_t after_ts;
    qp_obj_t qp_after_ts;

    if (!qp_is_int(qp_next(unp, &qp_after_ts)))
    {
        log_critical("job `del_expired`: invalid format");
        return -1;
    }

    after_ts = (uint64_t) qp_after_ts.via.int64;

    ti_users_del_expired(after_ts);
    return 0;
}

/*
 * Returns 0 on success
 * - for example: 'name'
 */
static int rjob__del_procedure(qp_unpacker_t * unp)
{
    assert (unp);

    qp_obj_t qp_name;
    ti_procedure_t * procedure;

    if (!qp_is_raw(qp_next(unp, &qp_name)))
    {
        log_critical("job `del_procedure`: missing procedure name");
        return -1;
    }

    procedure = ti_procedures_pop_strn(
            ti()->procedures,
            (const char *) qp_name.via.raw,
            qp_name.len);

    if (!procedure)
    {
        log_critical(
                "job `del_procedure` cannot find `%.*s`",
                (int) qp_name.len, (const char *) qp_name.via.raw);
        return -1;
    }

    ti_procedure_destroy(procedure);
    return 0;  /* success */
}

/*
 * Returns 0 on success
 * - for example: 'key'
 */
static int rjob__del_token(qp_unpacker_t * unp)
{
    assert (unp);
    qp_obj_t qp_key;
    ti_token_t * token;

    if (    !qp_is_raw(qp_next(unp, &qp_key)) ||
            qp_key.len != sizeof(ti_token_key_t))
    {
        log_critical(
                "job `del_token` for `.thingsdb`: "
                "missing or invalid token key");
        return -1;
    }

    token = ti_users_pop_token_by_key((ti_token_key_t *) qp_key.via.raw);

    if (!token)
    {
        log_critical("job `del_token` for `.thingsdb`: "
                "token key `%.*s` not found",
                (int) qp_key.len, (char *) qp_key.via.raw);
        return -1;
    }

    ti_token_destroy(token);
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
            !qp_is_int(qp_next(unp, &qp_target)) ||     /* value: target */
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: user */
            !qp_is_int(qp_next(unp, &qp_user)) ||       /* value: user */
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: mask */
            !qp_is_int(qp_next(unp, &qp_mask)))         /* value: mask */
    {
        log_critical("job `grant`: invalid format");
        return -1;
    }

    if (qp_target.via.int64 > 1)
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

    if (ti_access_grant(target
            ? &target->access
            : qp_target.via.int64 == TI_SCOPE_NODE
            ? &ti()->access_node
            : &ti()->access_thingsdb,
              user, mask))
    {
        log_critical(EX_MEMORY_S);
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
    ex_t e = {0};
    qp_obj_t qp_name, qp_user, qp_root;
    uint64_t user_id, root_id;
    ti_user_t * user;
    ti_collection_t * collection;

    if (    !qp_is_map(qp_next(unp, NULL)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: name */
            !qp_is_raw(qp_next(unp, &qp_name)) ||       /* value: name */
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: user */
            !qp_is_int(qp_next(unp, &qp_user)) ||       /* value: user */
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: root */
            !qp_is_int(qp_next(unp, &qp_root)))         /* value: root */
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
            &e);
    if (!collection)
    {
        log_critical("job `new_collection`: %s", e.msg);
        return -1;
    }

    return 0;
}

/*
 * Returns 0 on success
 * - for example: {
 *      'id': id,
 *      'port': port,
 *      'addr':ip_addr,
 *      'secret': encrypted
 *   }
 */
static int rjob__new_node(ti_event_t * ev, qp_unpacker_t * unp)
{
    assert (unp);
    qp_obj_t qp_id, qp_port, qp_addr, qp_secret;
    uint8_t node_id, next_node_id = ti()->nodes->vec->n;
    uint16_t port;
    char addr[INET6_ADDRSTRLEN];

    assert (((ti_node_t *) vec_last(ti()->nodes->vec))->id == next_node_id-1);

    if (    !qp_is_map(qp_next(unp, NULL)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||
            !qp_is_int(qp_next(unp, &qp_id)) ||
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

    qp_unpacker_print(unp);

    if (ev->id <= ti_.last_event_id)
        return 0;  /* this job is already applied */

    node_id = (uint8_t) qp_id.via.int64;
    port = (uint16_t) qp_port.via.int64;

    if (node_id != next_node_id)
    {
        log_critical(
                "job `new_node`: expecting "TI_NODE_ID" but got "TI_NODE_ID,
                next_node_id, node_id);
        return -1;
    }

    memcpy(addr, qp_addr.via.raw, qp_addr.len);
    addr[qp_addr.len] = '\0';

    if (!ti_nodes_new_node(0, port, addr, (const char *) qp_secret.via.raw))
    {
        log_critical(EX_MEMORY_S);
        return -1;
    }

    ev->flags |= TI_EVENT_FLAG_SAVE;

    return 0;
}

/*
 * Returns 0 on success
 * - for example: 'def'
 */
static int rjob__new_procedure(qp_unpacker_t * unp)
{
    assert (unp);

    int rc;
    qp_obj_t qp_name;
    ti_procedure_t * procedure;
    ti_closure_t * closure;
    ti_raw_t * rname;

    if (!qp_is_map(qp_next(unp, NULL)) ||
        !qp_is_raw(qp_next(unp, &qp_name)))
    {
        log_critical(
                "job `new_procedure` for `.thingsdb`: "
                "missing map or name");
        return -1;
    }


    rname = ti_raw_create(qp_name.via.raw, qp_name.len);
    closure = (ti_closure_t *) ti_val_from_unp(unp, NULL);
    procedure = NULL;

    if (!rname || !closure || !ti_val_is_closure((ti_val_t *) closure) ||
        !(procedure = ti_procedure_create(rname, closure)))
        goto failed;


    rc = ti_procedures_add(&ti()->procedures, procedure);
    if (rc == 0)
    {
        ti_decref(rname);
        ti_decref(closure);

        return 0;  /* success */
    }

    if (rc < 0)
        log_critical(EX_MEMORY_S);
    else
        log_critical(
                "job `new_procedure` for `.thingsdb`: "
                "procedure `%.*s` already exists",
                (int) procedure->name->n, (char *) procedure->name->data);

failed:
    ti_procedure_destroy(procedure);
    ti_val_drop((ti_val_t *) rname);
    ti_val_drop((ti_val_t *) closure);
    return -1;
}

/*
 * Returns 0 on success
 * - for example: {'id': id, 'key': value}, 'expire_ts': ts, 'description':..}
 */
static int rjob__new_token(qp_unpacker_t * unp)
{
    qp_obj_t qp_user, qp_key, qp_expire, qp_desc;
    uint64_t user_id;
    ti_user_t * user;
    ti_token_t * token;

    if (    !qp_is_map(qp_next(unp, NULL)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: id */
            !qp_is_int(qp_next(unp, &qp_user)) ||       /* value: id */
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: key */
            !qp_is_raw(qp_next(unp, &qp_key)) ||        /* value: key */
            qp_key.len != sizeof(ti_token_key_t) ||
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: expire_ts */
            !qp_is_int(qp_next(unp, &qp_expire)) ||     /* value: expire_ts */
            qp_expire.via.int64 < 0 ||
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: description */
            !qp_is_raw(qp_next(unp, &qp_desc)))         /* value: description */
    {
        log_critical("job `new_token`: invalid format");
        return -1;
    }

    user_id = (uint64_t) qp_user.via.int64;
    user = ti_users_get_by_id(user_id);
    if (!user)
    {
        log_critical("job `new_token`: "TI_USER_ID" not found", user_id);
        return -1;
    }

    token = ti_token_create(
            (ti_token_key_t *) qp_key.via.raw,
            qp_expire.via.int64,
            (const char *) qp_desc.via.raw,
            qp_desc.len);
    if (!token || ti_user_add_token(user, token))
    {
        ti_token_destroy(token);
        log_critical(EX_MEMORY_S);
        return -1;
    }

    return 0;
}

/*
 * Returns 0 on success
 * - for example: {'id': id, 'username':value}
 */
static int rjob__new_user(qp_unpacker_t * unp)
{
    ex_t e = {0};
    qp_obj_t qp_id, qp_name;
    uint64_t user_id;

    if (    !qp_is_map(qp_next(unp, NULL)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: id */
            !qp_is_int(qp_next(unp, &qp_id)) ||         /* value: id */
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: username */
            !qp_is_raw(qp_next(unp, &qp_name)))         /* value: username */
    {
        log_critical("job `new_user`: invalid format");
        return -1;
    }

    user_id = (uint64_t) qp_id.via.int64;

    if (!ti_users_load_user(
            user_id,
            (const char *) qp_name.via.raw,
            qp_name.len,
            NULL,
            &e))
    {
        log_critical("job `new_user`: %s", e.msg);
        return -1;
    }

    return 0;
}

/*
 * Returns 0 on success
 * - for example: id
 */
static int rjob__pop_node(ti_event_t * ev, qp_unpacker_t * unp)
{
    assert (unp);

    ti_node_t * this_node = ti()->node;
    uint8_t node_id, last_node_id = ti()->nodes->vec->n - 1;
    qp_obj_t qp_node;

    if (!qp_is_int(qp_next(unp, &qp_node)))
    {
        log_critical("job `pop_node`: invalid format");
        return -1;
    }

    qp_unpacker_print(unp);

    if (ev->id <= ti_.last_event_id)
        return 0;   /* this job is already applied */

    node_id = (uint64_t) qp_node.via.int64;

    if (node_id != last_node_id || node_id == this_node->id)
    {
        log_critical("cannot pop node: "TI_NODE_ID, node_id);
        return -1;
    }

    ti_nodes_pop_node();

    ev->flags |= TI_EVENT_FLAG_SAVE;

    return 0;
}

/*
 * Returns 0 on success
 * - for example: {'id':id, 'name':name}
 */
static int rjob__rename_collection(qp_unpacker_t * unp)
{
    ex_t e = {0};
    ti_collection_t * collection;
    uint64_t id;
    qp_obj_t qp_id, qp_name;
    ti_raw_t * rname;

    if (    !qp_is_map(qp_next(unp, NULL)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||
            !qp_is_int(qp_next(unp, &qp_id)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||
            !qp_is_raw(qp_next(unp, &qp_name)))
    {
        log_critical("job `rename_collection`: invalid format");
        return -1;
    }

    id = qp_id.via.int64;
    collection = ti_collections_get_by_id(id);
    if (!collection)
    {
        log_critical(
                "job `rename_collection`: "TI_COLLECTION_ID" not found", id);
        return -1;
    }

    rname = ti_raw_create(qp_name.via.raw, qp_name.len);
    if (!rname)
    {
        ex_set_mem(&e);
        return -1;
    }

    assert (e.nr == 0);

    (void) ti_collection_rename(collection, rname, &e);
    ti_val_drop((ti_val_t *) rname);

    return e.nr;
}

/*
 * Returns 0 on success
 * - for example: {'id':id, 'name':name}
 */
static int rjob__rename_user(qp_unpacker_t * unp)
{
    ex_t e = {0};
    ti_user_t * user;
    uint64_t id;
    qp_obj_t qp_id, qp_name;
    ti_raw_t * rname;

    if (    !qp_is_map(qp_next(unp, NULL)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||
            !qp_is_int(qp_next(unp, &qp_id)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||
            !qp_is_raw(qp_next(unp, &qp_name)))
    {
        log_critical("job `rename_user`: invalid format");
        return -1;
    }

    id = qp_id.via.int64;
    user = ti_users_get_by_id(id);
    if (!user)
    {
        log_critical(
                "job `rename_user`: "TI_USER_ID" not found", id);
        return -1;
    }

    rname = ti_raw_create(qp_name.via.raw, qp_name.len);
    if (!rname)
    {
        ex_set_mem(&e);
        return -1;
    }

    assert (e.nr == 0);

    ti_user_rename(user, rname, &e);
    ti_val_drop((ti_val_t *) rname);

    return e.nr;
}

/*
 * Returns 0 on success
 * - for example: {
 *      'id': id,
 *      'port': port,
 *      'addr':ip_addr,
 *      'secret': encrypted
 *   }
 */
static int rjob__replace_node(ti_event_t * ev, qp_unpacker_t * unp)
{
    ex_t e = {0};
    qp_obj_t qp_id, qp_port, qp_addr, qp_secret;
    uint8_t node_id;
    ti_node_t * node;

    if (    !qp_is_map(qp_next(unp, NULL)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||
            !qp_is_int(qp_next(unp, &qp_id)) ||
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
        log_critical("job `replace_node`: invalid format");
        return -1;
    }

    qp_unpacker_print(unp);

    if (ev->id <= ti_.last_event_id)
        return 0;  /* this job is already applied */

    node_id = (uint8_t) qp_id.via.int64;
    node = ti_nodes_node_by_id(node_id);

    if (!node)
    {
        log_critical(
                "job `replace_node`: "TI_NODE_ID" out of range",
                node_id);
        return -1;
    }

    memcpy(node->addr, qp_addr.via.raw, qp_addr.len);
    node->addr[qp_addr.len] = '\0';
    node->port = (uint16_t) qp_port.via.int64;
    memcpy(node->secret, qp_secret.via.raw, qp_secret.len);

    if (ti_node_update_sockaddr(node, &e) == 0)
        ev->flags |= TI_EVENT_FLAG_SAVE;

    return e.nr;
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
            !qp_is_int(qp_next(unp, &qp_target)) ||     /* value: target */
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: user */
            !qp_is_int(qp_next(unp, &qp_user)) ||       /* value: user */
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: mask */
            !qp_is_int(qp_next(unp, &qp_mask)))         /* value: mask */
    {
        log_critical("job `revoke`: invalid format");
        return -1;
    }

    if (qp_target.via.int64 > 1)
    {
        uint64_t id = (uint64_t) qp_target.via.int64;
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

    ti_access_revoke(target
            ? target->access
            : qp_target.via.int64 == TI_SCOPE_NODE
            ? ti()->access_node
            : ti()->access_thingsdb,
              user, mask);

    return 0;
}

/*
 * Returns 0 on success
 * - for example: {'id':user_id, 'password': encpass/null}
 */
static int rjob__set_password(qp_unpacker_t * unp)
{
    assert (unp);
    qp_obj_t qp_user, qp_pass;
    uint64_t user_id;
    ti_user_t * user;
    char * encrypted = NULL;

    if (    !qp_is_map(qp_next(unp, NULL)) ||
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: id */
            !qp_is_int(qp_next(unp, &qp_user)) ||       /* value: id */
            !qp_is_raw(qp_next(unp, NULL)) ||           /* key: password */
            (   !qp_is_raw(qp_next(unp, &qp_pass)) &&   /* value: password */
                !qp_is_null(qp_pass.tp)                 /*        or null */
            ))
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

    if (qp_is_raw(qp_pass.tp))
    {
        encrypted = qpx_obj_raw_to_str(&qp_pass);
        if (!encrypted)
        {
            log_critical(EX_MEMORY_S);
            return -1;
        }
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

    uint64_t id;
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

    if (!qp_collection.via.int64)
    {
        log_critical("job `set_quota`: cannot set quota on root (target: 0)");
        return -1;
    }

    id = qp_collection.via.int64;
    collection = ti_collections_get_by_id(id);
    if (!collection)
    {
        log_critical("job `set_quota`: "TI_COLLECTION_ID" not found", id);
        return -1;
    }

    quota_tp = (ti_quota_enum_t) qp_quota_tp.via.int64;
    quota = (size_t) qp_quota.via.int64;
    ti_collection_set_quota(collection, quota_tp, quota);

    return 0;
}

int ti_rjob_run(ti_event_t * ev, qp_unpacker_t * unp)
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
        if (qpx_obj_eq_str(&qp_job_name, "del_expired"))
            return rjob__del_expired(unp);
        if (qpx_obj_eq_str(&qp_job_name, "del_procedure"))
            return rjob__del_procedure(unp);
        if (qpx_obj_eq_str(&qp_job_name, "del_token"))
            return rjob__del_token(unp);
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
            return rjob__new_node(ev, unp);
        if (qpx_obj_eq_str(&qp_job_name, "new_procedure"))
            return rjob__new_procedure(unp);
        if (qpx_obj_eq_str(&qp_job_name, "new_token"))
            return rjob__new_token(unp);
        if (qpx_obj_eq_str(&qp_job_name, "new_user"))
            return rjob__new_user(unp);
        break;
    case 'p':
        if (qpx_obj_eq_str(&qp_job_name, "pop_node"))
            return rjob__pop_node(ev, unp);
        break;
    case 'r':
        if (qpx_obj_eq_str(&qp_job_name, "rename_collection"))
            return rjob__rename_collection(unp);
        if (qpx_obj_eq_str(&qp_job_name, "rename_user"))
            return rjob__rename_user(unp);
        if (qpx_obj_eq_str(&qp_job_name, "replace_node"))
            return rjob__replace_node(ev, unp);
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
