/*
 * ti/ttask.c (thingsdb task)
 */
#include <assert.h>
#include <ti.h>
#include <ti/access.h>
#include <ti/auth.h>
#include <ti/modules.h>
#include <ti/procedure.h>
#include <ti/procedures.h>
#include <ti/qbind.h>
#include <ti/raw.inline.h>
#include <ti/restore.h>
#include <ti/scope.h>
#include <ti/task.t.h>
#include <ti/token.h>
#include <ti/ttask.h>
#include <ti/users.h>
#include <ti/val.h>
#include <ti/val.inline.h>
#include <ti/verror.h>
#include <ti/vtask.h>
#include <ti/vtask.inline.h>
#include <ti/whitelist.h>
#include <util/cryptx.h>
#include <util/fx.h>
#include <util/mpack.h>

/*
 * Returns 0 on success
 * - for example: id
 */
static int ttask__clear_users(mp_unp_t * up)
{
    if (mp_skip(up) != MP_BOOL)
    {
        log_critical("task `clear_users`: invalid format");
        return -1;
    }

    if (ti_users_clear())
    {
        log_critical("error while clearing users");
    }

    return 0;
}

static int ttask__take_access(mp_unp_t * up)
{
    mp_obj_t mp_id;
    ti_user_t * user;

    if (mp_next(up, &mp_id) != MP_U64)
    {
        log_critical("task `take_access`: invalid format");
        return -1;
    }

    user = ti_users_get_by_id(mp_id.via.u64);
    if (!user)
    {
        log_critical(
                "task `take_access`: "TI_USER_ID" not found",
                mp_id.via.u64);
        return -1;
    }

    if (ti_access_grant(&ti.access_thingsdb, user, TI_AUTH_MASK_FULL))
    {
        log_critical("failed to take thingsdb access");
    }

    if (ti_access_grant(&ti.access_node, user, TI_AUTH_MASK_FULL))
    {
        log_critical("failed to take node access");
    }

    for (vec_each(ti.collections->vec, ti_collection_t, c))
    {
        if (ti_access_grant(&c->access, user, TI_AUTH_MASK_FULL))
        {
            log_critical(
                    "failed to take collection access ("TI_COLLECTION_ID")",
                    c->id);
        }
    }

    return 0;
}

/*
 * Returns 0 on success
 * - for example: id
 */
static int ttask__del_collection(mp_unp_t * up)
{
    mp_obj_t mp_id;

    if (mp_next(up, &mp_id) != MP_U64)
    {
        log_critical("task `del_collection`: invalid format");
        return -1;
    }

    if (!ti_collections_del_collection(mp_id.via.u64))
    {
        log_critical(
                "task `del_collection`: "TI_COLLECTION_ID" not found",
                mp_id.via.u64);
        return -1;
    }

    return 0;
}

/*
 * Returns 0 on success
 * - for example: after_is
 */
static int ttask__del_expired(mp_unp_t * up)
{
    mp_obj_t mp_after_ts;

    if (mp_next(up, &mp_after_ts) != MP_U64)
    {
        log_critical("task `del_expired`: invalid format");
        return -1;
    }

    ti_users_del_expired(mp_after_ts.via.u64);
    return 0;
}

/*
 * Returns 0 on success
 * - for example: 'name'
 */
static int ttask__del_procedure(mp_unp_t * up)
{
    mp_obj_t mp_name;
    ti_procedure_t * procedure;

    if (mp_next(up, &mp_name) != MP_STR)
    {
        log_critical("task `del_procedure`: missing procedure name");
        return -1;
    }

    procedure = ti_procedures_by_strn(
            ti.procedures,
            mp_name.via.str.data,
            mp_name.via.str.n);

    if (!procedure)
    {
        log_critical(
                "task `del_procedure` cannot find `%.*s`",
                mp_name.via.str.n, mp_name.via.str.data);
        return -1;
    }

    (void) ti_procedures_pop(ti.procedures, procedure);

    ti_procedure_destroy(procedure);
    return 0;  /* success */
}

/*
 * Returns 0 on success
 * - for example: 'key'
 */
static int ttask__del_token(mp_unp_t * up)
{
    mp_obj_t mp_key;
    ti_token_t * token;

    if (mp_next(up, &mp_key) != MP_STR ||
        mp_key.via.str.n != sizeof(ti_token_key_t))
    {
        log_critical(
                "task `del_token` for `.thingsdb`: "
                "missing or invalid token key");
        return -1;
    }

    token = ti_users_pop_token_by_key((ti_token_key_t *) mp_key.via.str.data);

    if (!token)
    {
        log_critical("task `del_token` for `.thingsdb`: "
                "token key `%.*s` not found",
                mp_key.via.str.n, mp_key.via.str.data);
        return -1;
    }

    ti_token_destroy(token);
    return 0;
}

/*
 * Returns 0 on success
 * - for example: id
 */
static int ttask__del_user(mp_unp_t * up)
{
    ti_user_t * user;
    mp_obj_t mp_id;

    if (mp_next(up, &mp_id) != MP_U64)
    {
        log_critical("task `del_user`: invalid format");
        return -1;
    }

    user = ti_users_get_by_id(mp_id.via.u64);
    if (!user)
    {
        log_critical("task `del_user`: "TI_USER_ID" not found", mp_id.via.u64);
        return -1;
    }

    ti_users_del_user(user);
    return 0;
}

/*
 * Returns 0 on success
 * - for example: {'scope':id, 'user':name, 'mask': integer}
 */
static int ttask__grant(mp_unp_t * up)
{
    ti_user_t * user;
    ti_collection_t * collection = NULL;
    mp_obj_t obj, mp_scope, mp_user, mp_mask;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 3 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_scope) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_user) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_mask) != MP_U64)
    {
        log_critical("task `grant`: invalid format");
        return -1;
    }

    if (mp_scope.via.u64 > 1)
    {
        collection = ti_collections_get_by_id(mp_scope.via.u64);
        if (!collection)
        {
            log_critical(
                    "task `grant`: "TI_COLLECTION_ID" not found",
                    mp_scope.via.u64);
            return -1;
        }
    }

    user = ti_users_get_by_id(mp_user.via.u64);
    if (!user)
    {
        log_critical("task `grant`: "TI_USER_ID" not found", mp_user.via.u64);
        return -1;
    }

    if (ti_access_grant(collection
            ? &collection->access
            : mp_scope.via.u64 == TI_SCOPE_NODE
            ? &ti.access_node
            : &ti.access_thingsdb,
              user, mp_mask.via.u64))
    {
        log_critical(EX_MEMORY_S);
        return -1;
    }

    return 0;
}

static int ttask__new_collection(mp_unp_t * up)
{
    ex_t e = {0};
    mp_obj_t obj, mp_name, mp_user, mp_root, mp_nfid, mp_created;
    ti_user_t * user;
    ti_collection_t * collection;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 5 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_user) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_root) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_nfid) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_created) != MP_U64)
    {
        /*
         * TODO (COMPAT): for compatibility with version before v1.5.0
         *                In previous versions the root Id (the first next
         *                free Id will be used for root) was equal to the
         *                collection Id as all Id's were unique across all
         *                existing collections; From version 1.5.0 this is
         *                changed and Id's are now only unique within a
         *                single collection.
         */
        if (obj.tp != MP_MAP || obj.via.sz != 4 ||
            mp_skip(up) != MP_STR ||
            mp_next(up, &mp_name) != MP_STR ||
            mp_skip(up) != MP_STR ||
            mp_next(up, &mp_user) != MP_U64 ||
            mp_skip(up) != MP_STR ||
            mp_next(up, &mp_root) != MP_U64 ||
            mp_skip(up) != MP_STR ||
            mp_next(up, &mp_created) != MP_U64)

        {
            log_critical("task `new_collection`: invalid format");
            return -1;
        }
        mp_nfid.tp = mp_root.tp;
        mp_nfid.via.u64 = mp_root.via.u64;
    }

    user = ti_users_get_by_id(mp_user.via.u64);
    if (!user)
    {
        log_critical(
                "task `new_collection`: "TI_USER_ID" not found",
                mp_user.via.u64);
        return -1;
    }

    collection = ti_collections_create_collection(
            mp_root.via.u64,
            mp_nfid.via.u64,
            mp_name.via.str.data,
            mp_name.via.str.n,
            mp_created.via.u64,
            user,
            &e);
    if (!collection)
    {
        log_critical("task `new_collection`: %s", e.msg);
        return -1;
    }

    return 0;
}

/*
 * Returns 0 on success
 * - for example: {
 *          'name': module_name,
 *          'file': file_name,
 *          'created_at': ts,
 *          'conf_pkg': configuration_package or nil,
 *          'scope_id': scope_id or nil}
 */
static int ttask__new_module(mp_unp_t * up)
{
    mp_obj_t obj, mp_name, mp_file, mp_created, mp_pkg, mp_scope;
    ti_module_t * module;
    uint64_t * scope_id = NULL;
    ti_pkg_t * conf_pkg = NULL;
    ex_t e = {0};

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 5 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_file) != MP_STR ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_created) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_pkg) <= 0 ||
        (mp_pkg.tp != MP_NIL && mp_pkg.tp != MP_BIN) ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_scope) <= 0 ||
        (mp_scope.tp != MP_NIL && mp_scope.tp != MP_U64))
    {
        log_critical("task `new_module`: invalid format");
        return -1;
    }

    if (mp_pkg.tp == MP_BIN)
    {
        conf_pkg = malloc(mp_pkg.via.bin.n);
        if (!conf_pkg)
            goto mem_error;
        memcpy(conf_pkg, mp_pkg.via.bin.data, mp_pkg.via.bin.n);
    }

    if (mp_scope.tp == MP_U64)
    {
        scope_id = malloc(sizeof(uint64_t));
        if (!scope_id)
            goto mem_error;
        *scope_id = mp_scope.via.u64;
    }

    module = ti_module_create(
            mp_name.via.str.data,
            mp_name.via.str.n,
            mp_file.via.str.data,
            mp_file.via.str.n,
            mp_created.via.u64,
            conf_pkg,
            scope_id,
            &e);
    if (!module)
    {
        log_critical("module already exist or allocation error");
        goto failed;
    }

    ti_module_load(module);
    return 0;

mem_error:
    log_critical(EX_MEMORY_S);
failed:
    free(conf_pkg);
    free(scope_id);
    return -1;
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
static int ttask__new_node(ti_change_t * change, mp_unp_t * up)
{
    mp_obj_t obj, mp_id, mp_port, mp_addr, mp_secret;
    char addr[INET6_ADDRSTRLEN];

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 4 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_port) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_addr) != MP_STR ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_secret) != MP_STR)
    {
        log_critical("task `new_node`: invalid format");
        return -1;
    }

    if (mp_addr.via.str.n >= INET6_ADDRSTRLEN)
    {
        log_critical("task `new_node`: invalid address size");
        return -1;
    }

    if (mp_secret.via.str.n != CRYPTX_SZ ||
        mp_secret.via.str.data[mp_secret.via.str.n-1] != '\0')
    {
        log_critical("task `new_node`: invalid secret");
        return -1;
    }

    if (change->id <= ti.last_change_id)
        return 0;  /* this task is already applied */

    memcpy(addr, mp_addr.via.str.data, mp_addr.via.str.n);
    addr[mp_addr.via.str.n] = '\0';

    if (!ti_nodes_new_node(
            mp_id.via.u64,
            0,
            mp_port.via.u64,
            addr,
            mp_secret.via.str.data))
    {
        log_critical(EX_MEMORY_S);
        return -1;
    }

    change->flags |= TI_CHANGE_FLAG_SAVE;

    return 0;
}

/*
 * Returns 0 on success
 * - for example: {...}
 */
static int ttask__new_procedure(mp_unp_t * up)
{
    int rc;
    mp_obj_t obj, mp_name, mp_created;
    ti_procedure_t * procedure;
    ti_closure_t * closure;
    ti_vup_t vup = {
            .isclient = false,
            .collection = NULL,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 3 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_created) != MP_U64 ||
        mp_skip(up) != MP_STR)
    {
        log_critical(
                "task `new_procedure` for `.thingsdb`: "
                "missing map or name");
        return -1;
    }

    closure = (ti_closure_t *) ti_val_from_vup(&vup);
    procedure = NULL;

    if (!closure || !ti_val_is_closure((ti_val_t *) closure) ||
        !(procedure = ti_procedure_create(
                mp_name.via.str.data,
                mp_name.via.str.n,
                closure,
                mp_created.via.u64)))
        goto failed;


    rc = ti_procedures_add(ti.procedures, procedure);
    if (rc == 0)
    {
        ti_decref(closure);

        return 0;  /* success */
    }

    if (rc < 0)
        log_critical(EX_MEMORY_S);
    else
        log_critical(
                "task `new_procedure` for `.thingsdb`: "
                "procedure `%s` already exists",
                procedure->name);

failed:
    ti_procedure_destroy(procedure);
    ti_val_drop((ti_val_t *) closure);
    return -1;
}

/*
 * Returns 0 on success
 * - for example: '{name: closure}'
 */
static int ttask__mod_procedure(mp_unp_t * up)
{
    mp_obj_t obj, mp_name, mp_created;
    ti_procedure_t * procedure;
    ti_closure_t * closure;
    ti_vup_t vup = {
            .isclient = false,
            .collection = NULL,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 3 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_created) != MP_U64 ||
        mp_skip(up) != MP_STR)
    {
        log_critical(
                "task `mod_procedure` for `.thingsdb`: missing map or name");
        return -1;
    }

    procedure = ti_procedures_by_strn(
            ti.procedures,
            mp_name.via.str.data,
            mp_name.via.str.n);

    if (!procedure)
    {
        log_critical(
                "task `mod_procedure` cannot find `%.*s` in `.thingsdb`",
                mp_name.via.str.n, mp_name.via.str.data);
        return -1;
    }

    closure = (ti_closure_t *) ti_val_from_vup(&vup);
    if (!closure || !ti_val_is_closure((ti_val_t *) closure))
    {
        log_critical(
                "task `mod_procedure` invalid closure in `.thingsdb`");
        ti_val_drop((ti_val_t *) closure);
        return -1;
    }

    ti_procedure_mod(procedure, closure, mp_created.via.u64);
    return 0;
}

static int ttask__whitelist_add(mp_unp_t * up)
{
    ex_t e = {0};
    mp_obj_t obj, mp_user, mp_wid;
    ti_user_t * user;
    ti_val_t * val = NULL;
    ti_vup_t vup = {
            .isclient = false,
            .collection = NULL,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_ARR || obj.via.sz < 2 || obj.via.sz > 3 ||
        mp_next(up, &mp_user) != MP_U64 ||
        mp_next(up, &mp_wid) != MP_U64 ||
        mp_wid.via.u64 < 0 || mp_wid.via.u64 > 1)
    {
        log_critical("task `whitelist_add`: invalid task data");
        return -1;
    }

    user = ti_users_get_by_id(mp_user.via.u64);
    if (!user)
    {
        log_critical(
                "task `whitelist_add`: "TI_USER_ID" not found",
                mp_user.via.u64);
        return -1;
    }

    if (obj.via.sz == 3)
    {
        val = (ti_val_t *) ti_val_from_vup(&vup);
        if (!val)
            ti_panic("failed to set whitelist value");
    }

    if (ti_whitelist_add(&user->whitelists[mp_wid.via.u64], val, &e))
    {
        log_critical("task `whitelist_add`: %s", e.msg);
        return -1;
    }
    return 0;
}

static int ttask__whitelist_drop(mp_unp_t * up)
{
    ex_t e = {0};
    mp_obj_t obj, mp_user, mp_wid;
    ti_user_t * user;
    ti_val_t * val = NULL;
    ti_vup_t vup = {
            .isclient = false,
            .collection = NULL,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_ARR || obj.via.sz < 2 || obj.via.sz > 3 ||
        mp_next(up, &mp_user) != MP_U64 ||
        mp_next(up, &mp_wid) != MP_U64 ||
        mp_wid.via.u64 < 0 || mp_wid.via.u64 > 1)
    {
        log_critical("task `whitelist_drop`: invalid task data");
        return -1;
    }

    user = ti_users_get_by_id(mp_user.via.u64);
    if (!user)
    {
        log_critical(
                "task `whitelist_drop`: "TI_USER_ID" not found",
                mp_user.via.u64);
        return -1;
    }

    if (obj.via.sz == 3)
    {
        val = (ti_val_t *) ti_val_from_vup(&vup);
        if (!val)
            ti_panic("failed to drop whitelist value");
    }

    if (ti_whitelist_drop(&user->whitelists[mp_wid.via.u64], val, &e))
    {
        log_critical("task `whitelist_drop`: %s", e.msg);
        return -1;
    }
    return 0;
}


/*
 * Returns 0 on success
 * - for example: {'id': id, 'key': value}, 'expire_ts': ts, 'description':..}
 */
static int ttask__new_token(mp_unp_t * up)
{
    mp_obj_t obj, mp_user, mp_key, mp_expire, mp_desc, mp_created;
    ti_user_t * user;
    ti_token_t * token;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 5 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_user) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_key) != MP_STR ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_expire) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_created) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_desc) != MP_STR)
    {
        log_critical("task `new_token`: invalid format");
        return -1;
    }

    if (mp_key.via.str.n != sizeof(ti_token_key_t))
    {
        log_critical("task `new_token`: invalid key size");
        return -1;
    }

    user = ti_users_get_by_id(mp_user.via.u64);
    if (!user)
    {
        log_critical(
                "task `new_token`: "TI_USER_ID" not found",
                mp_user.via.u64);
        return -1;
    }

    token = ti_token_create(
            (ti_token_key_t *) mp_key.via.str.data,
            mp_expire.via.u64,
            mp_created.via.u64,
            mp_desc.via.str.data,
            mp_desc.via.str.n);
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
static int ttask__new_user(mp_unp_t * up)
{
    ex_t e = {0};
    mp_obj_t obj, mp_id, mp_name, mp_created;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 3 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_created) != MP_U64)
    {
        log_critical("task `new_user`: invalid format");
        return -1;
    }

    if (!ti_users_load_user(
            mp_id.via.u64,
            mp_name.via.str.data,
            mp_name.via.str.n,
            NULL,
            mp_created.via.u64,
            &e))
    {
        log_critical("task `new_user`: %s", e.msg);
        return -1;
    }

    return 0;
}

/*
 * Returns 0 on success
 * - for example: id
 */
static int ttask__del_node(ti_change_t * change, mp_unp_t * up)
{
    ti_node_t * this_node = ti.node;
    mp_obj_t mp_node;

    if (mp_next(up, &mp_node) != MP_U64)
    {
        log_critical("task `del_node`: invalid format");
        return -1;
    }

    if (change->id <= ti.last_change_id)
        return 0;   /* this task is already applied */

    if (mp_node.via.u64 == this_node->id)
    {
        log_critical("cannot delete node with id %"PRIu64, mp_node.via.u64);
        return -1;
    }

    ti_nodes_del_node(mp_node.via.u64);

    change->flags |= TI_CHANGE_FLAG_SAVE;

    return 0;
}

/*
 * Returns 0 on success
 * - for example: module_name
 */
static int ttask__del_module(ti_change_t * change, mp_unp_t * up)
{
    ti_module_t * module;
    mp_obj_t mp_name;

    if (mp_next(up, &mp_name) != MP_STR)
    {
        log_critical("task `del_module`: invalid format");
        return -1;
    }

    module = ti_modules_by_strn(mp_name.via.str.data, mp_name.via.str.n);
    if (!module)
    {
        log_error("task `del_module`: module `%.*s` not found",
                mp_name.via.str.n,
                mp_name.via.str.data);
        return 0;  /* error, but able to continue */
    }

    ti_module_del(module, change->id > ti.node->scid);
    return 0;
}

/*
 * Returns 0 on success
 * - for example: {"name": module_name, "data": content}
 */
static int ttask__deploy_module(mp_unp_t * up)
{
    ti_module_t * module;
    mp_obj_t obj, mp_name, mp_data;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_skip(up) != MP_STR ||
        (mp_next(up, &mp_data) != MP_BIN && mp_data.tp != MP_NIL))
    {
        log_critical("task `deploy_module`: invalid format");
        return -1;
    }

    module = ti_modules_by_strn(mp_name.via.str.data, mp_name.via.str.n);
    if (!module)
    {
        log_error("task `deploy_module`: module `%.*s` not found",
                mp_name.via.str.n,
                mp_name.via.str.data);
        return 0;  /* error, but able to continue */
    }

    (void) ti_module_deploy(
                module,
                mp_data.tp == MP_BIN ? mp_data.via.bin.data : NULL,
                mp_data.tp == MP_BIN ? mp_data.via.bin.n : 0);
    return 0;
}

/*
 * Returns 0 on success
 * - for example: {"name": module_name, "scope_id": nil/id}
 */
static int ttask__set_module_scope(mp_unp_t * up)
{
    ti_module_t * module;
    mp_obj_t obj, mp_name, mp_scope;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_skip(up) != MP_STR ||
        (mp_next(up, &mp_scope) != MP_U64 && mp_scope.tp != MP_NIL))
    {
        log_critical("task `set_module_scope`: invalid format");
        return -1;
    }

    module = ti_modules_by_strn(mp_name.via.str.data, mp_name.via.str.n);
    if (!module)
    {
        log_error("task `set_module_scope`: module `%.*s` not found",
                mp_name.via.str.n,
                mp_name.via.str.data);
        return 0;  /* error, but able to continue */
    }

    free(module->scope_id);
    if (mp_scope.tp == MP_U64)
    {
        module->scope_id = malloc(sizeof(uint64_t));
        if (!module->scope_id)
        {
            log_critical(EX_MEMORY_S);
            return 0;  /* error, but able to continue */
        }
        *module->scope_id = mp_scope.via.u64;
    }
    else
        module->scope_id = NULL;

    return 0;
}

/*
 * Returns 0 on success
 * - for example: '{id: 123, run_at: ...}'
 */
static int ttask__vtask_new(mp_unp_t * up)
{
    mp_obj_t obj, mp_id, mp_run_at, mp_user_id;
    ti_vtask_t * vtask = NULL;
    ti_varr_t * varr = NULL;
    ti_closure_t * closure;
    ti_user_t * user;
    ti_vup_t vup = {
            .isclient = false,
            .collection = NULL,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 5 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_run_at) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_user_id) != MP_U64 ||
        mp_skip(up) != MP_STR)
    {
        log_critical(
                "task `vtask_new` for the `@thingsdb` scope: invalid data");
        return -1;
    }

    user = ti_users_get_by_id(mp_user_id.via.u64);
    closure = (ti_closure_t *) ti_val_from_vup(&vup);

    if (!closure || !ti_val_is_closure((ti_val_t *) closure) ||
        mp_skip(up) != MP_STR ||
        vec_reserve(&ti.tasks->vtasks, 1))
        goto fail0;

    varr = (ti_varr_t *) ti_val_from_vup(&vup);
    if (!varr || !ti_val_is_array((ti_val_t *) varr))
        goto fail0;

    vtask = ti_vtask_create(
                mp_id.via.u64,
                mp_run_at.via.u64,
                user,
                closure,
                NULL,
                varr->vec);
    if (!vtask)
        goto fail0;

    ti_update_next_free_id(vtask->id);
    VEC_push(ti.tasks->vtasks, vtask);
    free(varr);
    ti_decref(closure);
    return 0;

fail0:
    log_critical("task `vtask_new` for for the `@thingsdb` scope has failed");
    ti_val_drop((ti_val_t *) varr);
    ti_val_drop((ti_val_t *) closure);
    return -1;
}

/*
 * Returns 0 on success
 * - for example: '{id: 123}'
 */
static int ttask__vtask_del(mp_unp_t * up)
{
    mp_obj_t obj, mp_id;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64)
    {
        log_critical(
                "task `vtask_del` for the `@thingsdb` scope: invalid data");
        return -1;
    }

    ti_vtask_del(mp_id.via.u64, NULL);
    return 0;
}

/*
 * Returns 0 on success
 * - for example: '{id: 123}'
 */
static int ttask__vtask_cancel(mp_unp_t * up)
{
    mp_obj_t obj, mp_id;
    ti_vtask_t * vtask;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64)
    {
        log_critical(
                "task `vtask_cancel` for the `@thingsdb` scope: invalid data");
        return -1;
    }

    vtask = ti_vtask_by_id(ti.tasks->vtasks, mp_id.via.u64);
    if (vtask)
        ti_vtask_cancel(vtask);
    return 0;
}

static int ttask__vtask_finish(mp_unp_t * up)
{
    mp_obj_t obj, mp_id, mp_run_at;
    ti_val_t * val;
    ti_vtask_t * vtask;
    ti_vup_t vup = {
            .isclient = false,
            .collection = NULL,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 3 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_run_at) != MP_U64 ||
        mp_skip(up) != MP_STR)
    {
        log_critical(
            "task `vtask_finish` for the `@thingsdb` scope: invalid data");
        return -1;
    }

    val = ti_val_from_vup(&vup);
    if (!val)
        goto fail0;

    if (ti_val_is_nil(val))
    {
        ti_decref(val);
        val = (ti_val_t *) ti_verror_from_code(EX_SUCCESS);
    }
    else if (!ti_val_is_error(val))
        goto fail0;

    vtask = ti_vtask_by_id(ti.tasks->vtasks, mp_id.via.u64);
    if (!vtask)
        goto fail0;

    vtask->run_at = mp_run_at.via.u64;
    ti_tasks_vtask_finish(vtask);

    ti_val_drop((ti_val_t *) vtask->verr);
    vtask->verr = (ti_verror_t *) val;
    return 0;

fail0:
    log_critical(
            "task `vtask_finish` for the `@thingsdb` scope has failed");
    ti_val_drop(val);
    return -1;
}

static int ttask__vtask_set_args(mp_unp_t * up)
{
    mp_obj_t obj, mp_id;
    ti_varr_t * varr;
    ti_vtask_t * vtask;
    ti_vup_t vup = {
            .isclient = false,
            .collection = NULL,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR)
    {
        log_critical(
            "task `vtask_set_args` for the `@thingsdb` scope: invalid data");
        return -1;
    }

    varr = (ti_varr_t *) ti_val_from_vup(&vup);
    if (!varr || !ti_val_is_array((ti_val_t *) varr))
        goto fail0;

    vtask = ti_vtask_by_id(ti.tasks->vtasks, mp_id.via.u64);
    if (vtask)
    {
        vec_t * tmp = vtask->args;
        vtask->args = varr->vec;
        varr->vec = tmp;
    }

    ti_val_unsafe_drop((ti_val_t *) varr);
    return 0;

fail0:
    log_critical(
            "task `vtask_set_args` for the `@thingsdb` scope has failed");
    ti_val_drop((ti_val_t *) varr);
    return -1;
}

static int ttask__vtask_set_owner(mp_unp_t * up)
{
    mp_obj_t obj, mp_id, mp_user_id;
    ti_user_t * user;
    ti_vtask_t * vtask;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_user_id) != MP_U64)
    {
        log_critical(
            "task `vtask_set_owner` for the `@thingsdb` scope: invalid data");
        return -1;
    }

    user = ti_users_get_by_id(mp_user_id.via.u64);
    vtask = ti_vtask_by_id(ti.tasks->vtasks, mp_id.via.u64);
    if (vtask)
    {
        ti_user_drop(vtask->user);
        vtask->user = user;
        ti_incref(user);
    }

    return 0;
}

static int ttask__vtask_set_closure(mp_unp_t * up)
{
    mp_obj_t obj, mp_id;
    ti_closure_t * closure;
    ti_vtask_t * vtask;
    ti_vup_t vup = {
            .isclient = false,
            .collection = NULL,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR)
    {
        log_critical(
            "task `vtask_set_closure` for the `@thingsdb` scope: invalid data");
        return -1;
    }

    closure = (ti_closure_t *) ti_val_from_vup(&vup);
    if (!closure || !ti_val_is_closure((ti_val_t *) closure))
        goto fail0;

    vtask = ti_vtask_by_id(ti.tasks->vtasks, mp_id.via.u64);
    if (vtask && ti_vtask_set_closure(vtask, closure) == 0)
        return 0;

    ti_val_unsafe_drop((ti_val_t *) closure);

    if (!vtask)
        return 0;
    closure = NULL;

fail0:
    log_critical(
            "task `vtask_set_closure` for the `@thingsdb` scope has failed");
    ti_val_drop((ti_val_t *) closure);
    return -1;
}

/* TODO (COMPAT): Obsolete task */
static int ttask__del_timer(mp_unp_t * up)
{
    log_warning("task `del_timer` is obsolete");
    mp_skip(up);
    return 0;
}

/* TODO (COMPAT): Obsolete task */
static int ttask__new_timer(mp_unp_t * up)
{
    log_warning("task `new_timer` is obsolete");
    mp_skip(up);
    return 0;
}

/* TODO (COMPAT): Obsolete task */
static int ttask__set_timer_args(mp_unp_t * up)
{
    log_warning("task `set_timer_args` is obsolete");
    mp_skip(up);
    return 0;
}

/*
 * Returns 0 on success
 * - for example: {"name": module_name, "conf_pkg": nil/bin}
 */
static int ttask__set_module_conf(mp_unp_t * up)
{
    ti_module_t * module;
    mp_obj_t obj, mp_name, mp_pkg;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_skip(up) != MP_STR ||
        (mp_next(up, &mp_pkg) != MP_BIN && mp_pkg.tp != MP_NIL))
    {
        log_critical("task `set_module_conf`: invalid format");
        return -1;
    }

    module = ti_modules_by_strn(mp_name.via.str.data, mp_name.via.str.n);
    if (!module)
    {
        log_error("task `set_module_conf`: module `%.*s` not found",
                mp_name.via.str.n,
                mp_name.via.str.data);
        return 0;  /* error, but able to continue */
    }

    free(module->conf_pkg);
    if (mp_pkg.tp == MP_BIN)
    {
        module->conf_pkg = malloc(mp_pkg.via.bin.n);
        if (!module->conf_pkg)
        {
            log_critical(EX_MEMORY_S);
            return 0;  /* error, but able to continue */
        }
        memcpy(module->conf_pkg, mp_pkg.via.bin.data, mp_pkg.via.bin.n);
    }
    else
        module->conf_pkg = NULL;

    ti_module_update_conf(module);
    return 0;
}

/*
 * Returns 0 on success
 * - for example: {'id':id, 'name':name}
 */
static int ttask__rename_collection(mp_unp_t * up)
{
    ex_t e = {0};
    ti_collection_t * collection;
    mp_obj_t obj, mp_id, mp_name;
    ti_raw_t * rname;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR)
    {
        log_critical("task `rename_collection`: invalid format");
        return -1;
    }

    collection = ti_collections_get_by_id(mp_id.via.u64);
    if (!collection)
    {
        log_critical(
                "task `rename_collection`: "TI_COLLECTION_ID" not found",
                mp_id.via.u64);
        return -1;
    }

    rname = ti_str_create(mp_name.via.str.data, mp_name.via.str.n);
    if (!rname)
    {
        log_critical(EX_MEMORY_S);
        return -1;
    }

    (void) ti_collection_rename(collection, rname, &e);
    ti_val_drop((ti_val_t *) rname);

    return e.nr;
}

/*
 * Returns 0 on success
 * - for example: {'id':id, 'name':name}
 */
static int ttask__set_time_zone(mp_unp_t * up)
{
    mp_obj_t obj, mp_id, mp_tz;
    ti_tz_t * tz;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_tz) != MP_U64)
    {
        log_critical("task `set_time_zone`: invalid format");
        return -1;
    }

    tz = ti_tz_from_index(mp_tz.via.u64);
    ti_scope_set_tz(mp_id.via.u64, tz);

    return 0;
}

/*
 * Returns 0 on success
 * - for example: {'id':id, 'deep':1}
 */
static int ttask__set_default_deep(mp_unp_t * up)
{
    mp_obj_t obj, mp_id, mp_deep;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_deep) != MP_U64)
    {
        log_critical("task `set_default_deep`: invalid format");
        return -1;
    }

    ti_scope_set_deep(mp_id.via.u64, mp_deep.via.u64);
    return 0;
}

/*
 * Returns 0 on success
 * - for example: true
 */
static int ttask__restore_finished(mp_unp_t * up)
{
    mp_obj_t clear_tasks;
    if (mp_next(up, &clear_tasks) != MP_BOOL)
    {
        log_critical("task `restore_finished`: invalid format");
        return -1;
    }

    if (clear_tasks.via.bool_)
        ti_tasks_clear_all();

    ti_restore_finished();
    return 0;
}


/*
 * Returns 0 on success
 * - for example: {'old':name, 'name':name}
 */
static int ttask__rename_module(mp_unp_t * up)
{
    ti_module_t * module;
    mp_obj_t obj, mp_old, mp_name;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_old) != MP_STR ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR)
    {
        log_critical("task `rename_module`: invalid format");
        return -1;
    }

    module = ti_modules_by_strn(mp_old.via.str.data, mp_old.via.str.n);

    if (!module)
    {
        log_critical(
                "task `rename_module` cannot find `%.*s`",
                mp_old.via.str.n, mp_old.via.str.data);
        return -1;
    }

    return ti_modules_rename(module, mp_name.via.str.data, mp_name.via.str.n);
}

/*
 * Returns 0 on success
 * - for example: {'old':name, 'name':name}
 */
static int ttask__rename_procedure(mp_unp_t * up)
{
    ti_procedure_t * procedure;
    mp_obj_t obj, mp_old, mp_name;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_old) != MP_STR ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR)
    {
        log_critical("task `rename_procedure`: invalid format");
        return -1;
    }

    procedure = ti_procedures_by_strn(
            ti.procedures,
            mp_old.via.str.data,
            mp_old.via.str.n);

    if (!procedure)
    {
        log_critical(
                "task `rename_procedure` cannot find `%.*s`",
                mp_old.via.str.n, mp_old.via.str.data);
        return -1;
    }

    return ti_procedures_rename(
            ti.procedures,
            procedure,
            mp_name.via.str.data,
            mp_name.via.str.n);
}

/*
 * Returns 0 on success
 * - for example: {'id':id, 'name':name}
 */
static int ttask__rename_user(mp_unp_t * up)
{
    ex_t e = {0};
    ti_user_t * user;
    mp_obj_t obj, mp_id, mp_name;
    ti_raw_t * rname;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR)
    {
        log_critical("task `rename_user`: invalid format");
        return -1;
    }

    user = ti_users_get_by_id(mp_id.via.u64);
    if (!user)
    {
        log_critical(
                "task `rename_user`: "TI_USER_ID" not found", mp_id.via.u64);
        return -1;
    }

    rname = ti_str_create(mp_name.via.str.data, mp_name.via.str.n);
    if (!rname)
    {
        log_critical(EX_MEMORY_S);
        return -1;
    }

    (void) ti_user_rename(user, rname, &e);
    ti_val_drop((ti_val_t *) rname);

    return e.nr;
}

/*
 * Returns 0 on success
 * - for example: true
 */
static int ttask__restore(mp_unp_t * up)
{
    mp_obj_t obj;

    switch (mp_next(up, &obj))
    {
    case MP_BOOL:
        break;
    default:
        log_critical("task `restore`: invalid format");
        return -1;
    }

    if (fx_is_dir(ti.store->store_path))
    {
        log_warning("removing store directory: `%s`", ti.store->store_path);
        if (fx_rmdir(ti.store->store_path))
            log_error("failed to remove path: `%s`", ti.store->store_path);
    }

    if (ti_archive_rmdir())
        log_error("failed to remove archives");

    return ti_restore_slave();
}

/*
 * Returns 0 on success
 * - for example: {'scope':id, 'user':name, 'mask': integer}
 */
static int ttask__revoke(mp_unp_t * up)
{
    ti_user_t * user;
    ti_collection_t * collection = NULL;
    mp_obj_t obj, mp_scope, mp_user, mp_mask;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 3 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_scope) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_user) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_mask) != MP_U64)
    {
        log_critical("task `revoke`: invalid format");
        return -1;
    }

    if (mp_scope.via.u64 > 1)
    {
        collection = ti_collections_get_by_id(mp_scope.via.u64);
        if (!collection)
        {
            log_critical(
                    "task `revoke`: "TI_COLLECTION_ID" not found",
                    mp_scope.via.u64);
            return -1;
        }
    }

    user = ti_users_get_by_id(mp_user.via.u64);
    if (!user)
    {
        log_critical("task `revoke`: "TI_USER_ID" not found", mp_user.via.u64);
        return -1;
    }

    ti_access_revoke(collection
            ? collection->access
            : mp_scope.via.u64 == TI_SCOPE_NODE
            ? ti.access_node
            : ti.access_thingsdb,
              user, mp_mask.via.u64);

    return 0;
}

/*
 * Returns 0 on success
 * - for example: {'id':user_id, 'password': encpass/null}
 */
static int ttask__set_password(mp_unp_t * up)
{
    ti_user_t * user;
    mp_obj_t obj, mp_user, mp_pass;
    char * encrypted = NULL;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_user) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_pass) <= 0 ||
        (mp_pass.tp != MP_STR && mp_pass.tp != MP_NIL))
    {
        log_critical("task `set_password`: invalid format");
        return -1;
    }

    user = ti_users_get_by_id(mp_user.via.u64);
    if (!user)
    {
        log_critical(
                "task `set_password`: "TI_USER_ID" not found",
                mp_user.via.u64);
        return -1;
    }

    if (mp_pass.tp == MP_STR)
    {
        encrypted = mp_strdup(&mp_pass);
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

int ti_ttask_run(ti_change_t * change, mp_unp_t * up)
{
    mp_obj_t obj, mp_task;
    if (mp_next(up, &obj) != MP_ARR || obj.via.sz != 2 ||
        mp_next(up, &mp_task) != MP_U64)
    {
        if (obj.tp != MP_MAP || obj.via.sz != 1 ||
            mp_next(up, &mp_task) != MP_STR || mp_task.via.str.n < 3)
        {
            log_critical(
                    "task is not a `map` or `type` "
                    "for thing "TI_THING_ID" is missing", 0);
            return -1;
        }
        goto version_v0;
    }

    switch ((ti_task_enum) mp_task.via.u64)
    {
    case TI_TASK_SET:               break;
    case TI_TASK_DEL:               break;
    case TI_TASK_SPLICE:            break;
    case TI_TASK_SET_ADD:           break;
    case TI_TASK_SET_REMOVE:        break;
    case TI_TASK_DEL_COLLECTION:    return ttask__del_collection(up);
    case TI_TASK_DEL_ENUM:          break;
    case TI_TASK_DEL_EXPIRED:       return ttask__del_expired(up);
    case TI_TASK_DEL_MODULE:        return ttask__del_module(change, up);
    case TI_TASK_DEL_NODE:          return ttask__del_node(change, up);
    case TI_TASK_DEL_PROCEDURE:     return ttask__del_procedure(up);
    case _TI_TASK_DEL_TIMER:        return ttask__del_timer(up);
    case TI_TASK_DEL_TOKEN:         return ttask__del_token(up);
    case TI_TASK_DEL_TYPE:          break;
    case TI_TASK_DEL_USER:          return ttask__del_user(up);
    case TI_TASK_DEPLOY_MODULE:     return ttask__deploy_module(up);
    case TI_TASK_GRANT:             return ttask__grant(up);
    case TI_TASK_MOD_ENUM_ADD:      break;
    case TI_TASK_MOD_ENUM_DEF:      break;
    case TI_TASK_MOD_ENUM_DEL:      break;
    case TI_TASK_MOD_ENUM_MOD:      break;
    case TI_TASK_MOD_ENUM_REN:      break;
    case TI_TASK_MOD_TYPE_ADD:      break;
    case TI_TASK_MOD_TYPE_DEL:      break;
    case TI_TASK_MOD_TYPE_MOD:      break;
    case TI_TASK_MOD_TYPE_REL_ADD:  break;
    case TI_TASK_MOD_TYPE_REL_DEL:  break;
    case TI_TASK_MOD_TYPE_REN:      break;
    case TI_TASK_MOD_TYPE_WPO:      break;
    case TI_TASK_NEW_COLLECTION:    return ttask__new_collection(up);
    case TI_TASK_NEW_MODULE:        return ttask__new_module(up);
    case TI_TASK_NEW_NODE:          return ttask__new_node(change, up);
    case TI_TASK_NEW_PROCEDURE:     return ttask__new_procedure(up);
    case _TI_TASK_NEW_TIMER:        return ttask__new_timer(up);
    case TI_TASK_NEW_TOKEN:         return ttask__new_token(up);
    case TI_TASK_NEW_TYPE:          break;
    case TI_TASK_NEW_USER:          return ttask__new_user(up);
    case TI_TASK_RENAME_COLLECTION: return ttask__rename_collection(up);
    case TI_TASK_RENAME_ENUM:       break;
    case TI_TASK_RENAME_MODULE:     return ttask__rename_module(up);
    case TI_TASK_RENAME_PROCEDURE:  return ttask__rename_procedure(up);
    case TI_TASK_RENAME_TYPE:       break;
    case TI_TASK_RENAME_USER:       return ttask__rename_user(up);
    case TI_TASK_RESTORE:           return ttask__restore(up);
    case TI_TASK_REVOKE:            return ttask__revoke(up);
    case TI_TASK_SET_ENUM:          break;
    case TI_TASK_SET_MODULE_CONF:   return ttask__set_module_conf(up);
    case TI_TASK_SET_MODULE_SCOPE:  return ttask__set_module_scope(up);
    case TI_TASK_SET_PASSWORD:      return ttask__set_password(up);
    case TI_TASK_SET_TIME_ZONE:     return ttask__set_time_zone(up);
    case _TI_TASK_SET_TIMER_ARGS:   return ttask__set_timer_args(up);
    case TI_TASK_SET_TYPE:          break;
    case TI_TASK_TO_TYPE:           break;
    case TI_TASK_CLEAR_USERS:       return ttask__clear_users(up);
    case TI_TASK_TAKE_ACCESS:       return ttask__take_access(up);
    case TI_TASK_ARR_REMOVE:        break;
    case TI_TASK_THING_CLEAR:       break;
    case TI_TASK_ARR_CLEAR:         break;
    case TI_TASK_SET_CLEAR:         break;
    case TI_TASK_VTASK_NEW:         return ttask__vtask_new(up);
    case TI_TASK_VTASK_DEL:         return ttask__vtask_del(up);
    case TI_TASK_VTASK_CANCEL:      return ttask__vtask_cancel(up);
    case TI_TASK_VTASK_FINISH:      return ttask__vtask_finish(up);
    case TI_TASK_VTASK_SET_ARGS:    return ttask__vtask_set_args(up);
    case TI_TASK_VTASK_SET_OWNER:   return ttask__vtask_set_owner(up);
    case TI_TASK_VTASK_SET_CLOSURE: return ttask__vtask_set_closure(up);
    case TI_TASK_THING_RESTRICT:    break;
    case TI_TASK_THING_REMOVE:      break;
    case TI_TASK_SET_DEFAULT_DEEP:  return ttask__set_default_deep(up);
    case TI_TASK_RESTORE_FINISHED:  return ttask__restore_finished(up);
    case TI_TASK_TO_THING:          break;
    case TI_TASK_MOD_TYPE_HID:      break;
    case TI_TASK_REN:               break;
    case TI_TASK_FILL:              break;
    case TI_TASK_MOD_PROCEDURE:     return ttask__mod_procedure(up);
    case TI_TASK_NEW_ENUM:          break;
    case TI_TASK_SET_ENUM_DATA:     break;
    case TI_TASK_REPLACE_ROOT:      break;
    case TI_TASK_IMPORT:            break;
    case TI_TASK_ROOM_SET_NAME:     break;
    case TI_TASK_WHITELIST_ADD:     return ttask__whitelist_add(up);
    case TI_TASK_WHITELIST_DROP:    return ttask__whitelist_drop(up);
    }

    log_critical("unknown thingsdb task: %"PRIu64, mp_task.via.u64);
    return -1;

version_v0:
    switch (*mp_task.via.str.data)
    {
    case 'c':
        if (mp_str_eq(&mp_task, "clear_users"))
            return ttask__clear_users(up);
        break;
    case 'd':
        if (mp_str_eq(&mp_task, "del_timer"))
            return ttask__del_timer(up);
        if (mp_str_eq(&mp_task, "deploy_module"))
            return ttask__deploy_module(up);
        if (mp_str_eq(&mp_task, "del_collection"))
            return ttask__del_collection(up);
        if (mp_str_eq(&mp_task, "del_expired"))
            return ttask__del_expired(up);
        if (mp_str_eq(&mp_task, "del_node"))
            return ttask__del_node(change, up);
        if (mp_str_eq(&mp_task, "del_module"))
            return ttask__del_module(change, up);
        if (mp_str_eq(&mp_task, "del_procedure"))
            return ttask__del_procedure(up);
        if (mp_str_eq(&mp_task, "del_token"))
            return ttask__del_token(up);
        if (mp_str_eq(&mp_task, "del_user"))
            return ttask__del_user(up);
        break;
    case 'g':
        if (mp_str_eq(&mp_task, "grant"))
            return ttask__grant(up);
        break;
    case 'n':
        if (mp_str_eq(&mp_task, "new_timer"))
            return ttask__new_timer(up);
        if (mp_str_eq(&mp_task, "new_collection"))
            return ttask__new_collection(up);
        if (mp_str_eq(&mp_task, "new_module"))
            return ttask__new_module(up);
        if (mp_str_eq(&mp_task, "new_node"))
            return ttask__new_node(change, up);
        if (mp_str_eq(&mp_task, "new_procedure"))
            return ttask__new_procedure(up);
        if (mp_str_eq(&mp_task, "new_token"))
            return ttask__new_token(up);
        if (mp_str_eq(&mp_task, "new_user"))
            return ttask__new_user(up);
        break;
    case 'r':
        if (mp_str_eq(&mp_task, "rename_collection"))
            return ttask__rename_collection(up);
        if (mp_str_eq(&mp_task, "rename_module"))
            return ttask__rename_module(up);
        if (mp_str_eq(&mp_task, "rename_procedure"))
            return ttask__rename_procedure(up);
        if (mp_str_eq(&mp_task, "rename_user"))
            return ttask__rename_user(up);
        if (mp_str_eq(&mp_task, "restore"))
            return ttask__restore(up);
        if (mp_str_eq(&mp_task, "revoke"))
            return ttask__revoke(up);
        break;
    case 's':
        if (mp_str_eq(&mp_task, "set_timer_args"))
            return ttask__set_timer_args(up);
        if (mp_str_eq(&mp_task, "set_module_conf"))
            return ttask__set_module_conf(up);
        if (mp_str_eq(&mp_task, "set_module_scope"))
            return ttask__set_module_scope(up);
        if (mp_str_eq(&mp_task, "set_password"))
            return ttask__set_password(up);
        if (mp_str_eq(&mp_task, "set_time_zone"))
            return ttask__set_time_zone(up);
        if (mp_str_eq(&mp_task, "set_quota"))
        {
            /*
             * DEPRECATED: `set_quota` is removed, skip this task
             */
            mp_skip(up);
            return 0;
        }
        break;
    case 't':
        if (mp_str_eq(&mp_task, "take_access"))
            return ttask__take_access(up);
        break;
    }

    log_critical(
            "unknown thingsdb task: `%.*s`",
            mp_task.via.str.n,
            mp_task.via.str.data);
    return -1;
}
