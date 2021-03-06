/*
 * ti/rjob.c
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
#include <ti/rjob.h>
#include <ti/timer.h>
#include <ti/timer.inline.h>
#include <ti/token.h>
#include <ti/users.h>
#include <ti/val.h>
#include <ti/val.inline.h>
#include <util/cryptx.h>
#include <util/fx.h>
#include <util/mpack.h>

/*
 * Returns 0 on success
 * - for example: id
 */
static int rjob__clear_users(mp_unp_t * up)
{
    if (mp_skip(up) != MP_BOOL)
    {
        log_critical("job `clear_users`: invalid format");
        return -1;
    }

    if (ti_users_clear())
    {
        log_critical("error while clearing users");
    }

    return 0;
}

static int rjob__take_access(mp_unp_t * up)
{
    mp_obj_t mp_id;
    ti_user_t * user;

    if (mp_next(up, &mp_id) != MP_U64)
    {
        log_critical("job `take_access`: invalid format");
        return -1;
    }

    user = ti_users_get_by_id(mp_id.via.u64);
    if (!user)
    {
        log_critical(
                "job `take_access`: "TI_USER_ID" not found",
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
                    c->root->id);
        }
    }

    return 0;
}

/*
 * Returns 0 on success
 * - for example: id
 */
static int rjob__del_collection(mp_unp_t * up)
{
    mp_obj_t mp_id;

    if (mp_next(up, &mp_id) != MP_U64)
    {
        log_critical("job `del_collection`: invalid format");
        return -1;
    }

    if (!ti_collections_del_collection(mp_id.via.u64))
    {
        log_critical(
                "job `del_collection`: "TI_COLLECTION_ID" not found",
                mp_id.via.u64);
        return -1;
    }

    return 0;
}

/*
 * Returns 0 on success
 * - for example: after_is
 */
static int rjob__del_expired(mp_unp_t * up)
{
    mp_obj_t mp_after_ts;

    if (mp_next(up, &mp_after_ts) != MP_U64)
    {
        log_critical("job `del_expired`: invalid format");
        return -1;
    }

    ti_users_del_expired(mp_after_ts.via.u64);
    return 0;
}

/*
 * Returns 0 on success
 * - for example: 'name'
 */
static int rjob__del_procedure(mp_unp_t * up)
{
    mp_obj_t mp_name;
    ti_procedure_t * procedure;

    if (mp_next(up, &mp_name) != MP_STR)
    {
        log_critical("job `del_procedure`: missing procedure name");
        return -1;
    }

    procedure = ti_procedures_by_strn(
            ti.procedures,
            mp_name.via.str.data,
            mp_name.via.str.n);

    if (!procedure)
    {
        log_critical(
                "job `del_procedure` cannot find `%.*s`",
                mp_name.via.str.n, mp_name.via.str.data);
        return -1;
    }

    (void) ti_procedures_pop(ti.procedures, procedure);

    ti_procedure_destroy(procedure);
    return 0;  /* success */
}

/*
 * Returns 0 on success
 * - for example: 'id'
 */
static int rjob__del_timer(mp_unp_t * up)
{
    mp_obj_t mp_id;

    if (mp_next(up, &mp_id) != MP_U64)
    {
        log_critical("job `del_timer`: missing timer id");
        return -1;
    }

    for (vec_each(ti.timers->timers, ti_timer_t, timer))
    {
        if (timer->id == mp_id.via.u64)
        {
            ti_timer_mark_del(timer);
            break;
        }
    }

    /*
     * For a timer event it may occur that a timer is already marked for
     * deletion and the timer may even be removed.
     */
    return 0;
}

/*
 * Returns 0 on success
 * - for example: 'key'
 */
static int rjob__del_token(mp_unp_t * up)
{
    mp_obj_t mp_key;
    ti_token_t * token;

    if (mp_next(up, &mp_key) != MP_STR ||
        mp_key.via.str.n != sizeof(ti_token_key_t))
    {
        log_critical(
                "job `del_token` for `.thingsdb`: "
                "missing or invalid token key");
        return -1;
    }

    token = ti_users_pop_token_by_key((ti_token_key_t *) mp_key.via.str.data);

    if (!token)
    {
        log_critical("job `del_token` for `.thingsdb`: "
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
static int rjob__del_user(mp_unp_t * up)
{
    ti_user_t * user;
    mp_obj_t mp_id;

    if (mp_next(up, &mp_id) != MP_U64)
    {
        log_critical("job `del_user`: invalid format");
        return -1;
    }

    user = ti_users_get_by_id(mp_id.via.u64);
    if (!user)
    {
        log_critical("job `del_user`: "TI_USER_ID" not found", mp_id.via.u64);
        return -1;
    }

    ti_users_del_user(user);
    return 0;
}

/*
 * Returns 0 on success
 * - for example: {'scope':id, 'user':name, 'mask': integer}
 */
static int rjob__grant(mp_unp_t * up)
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
        log_critical("job `grant`: invalid format");
        return -1;
    }

    if (mp_scope.via.u64 > 1)
    {
        collection = ti_collections_get_by_id(mp_scope.via.u64);
        if (!collection)
        {
            log_critical(
                    "job `grant`: "TI_COLLECTION_ID" not found",
                    mp_scope.via.u64);
            return -1;
        }
    }

    user = ti_users_get_by_id(mp_user.via.u64);
    if (!user)
    {
        log_critical("job `grant`: "TI_USER_ID" not found", mp_user.via.u64);
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

/*
 * Returns 0 on success
 * - for example: {
 *          'name': collection_name,
 *          'user': id,
 *          'root': id,
 *          'created_at': ts}
 */
static int rjob__new_collection(mp_unp_t * up)
{
    ex_t e = {0};
    mp_obj_t obj, mp_name, mp_user, mp_root, mp_created;
    ti_user_t * user;
    ti_collection_t * collection;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 4 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_user) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_root) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_created) != MP_U64)
    {
        log_critical("job `new_collection`: invalid format");
        return -1;
    }

    user = ti_users_get_by_id(mp_user.via.u64);
    if (!user)
    {
        log_critical(
                "job `new_collection`: "TI_USER_ID" not found",
                mp_user.via.u64);
        return -1;
    }

    collection = ti_collections_create_collection(
            mp_root.via.u64,
            mp_name.via.str.data,
            mp_name.via.str.n,
            mp_created.via.u64,
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
 *          'name': module_name,
 *          'file': file_name,
 *          'created_at': ts,
 *          'conf_pkg': configuration_package or nil,
 *          'scope_id': scope_id or nil}
 */
static int rjob__new_module(mp_unp_t * up)
{
    mp_obj_t obj, mp_name, mp_file, mp_created, mp_pkg, mp_scope;
    ti_module_t * module;
    uint64_t * scope_id = NULL;
    ti_pkg_t * conf_pkg = NULL;

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
        log_critical("job `new_module`: invalid format");
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
            scope_id);

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
static int rjob__new_node(ti_event_t * ev, mp_unp_t * up)
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
        log_critical("job `new_node`: invalid format");
        return -1;
    }

    if (mp_addr.via.str.n >= INET6_ADDRSTRLEN)
    {
        log_critical("job `new_node`: invalid address size");
        return -1;
    }

    if (mp_secret.via.str.n != CRYPTX_SZ ||
        mp_secret.via.str.data[mp_secret.via.str.n-1] != '\0')
    {
        log_critical("job `new_node`: invalid secret");
        return -1;
    }

    if (ev->id <= ti.last_event_id)
        return 0;  /* this job is already applied */

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

    ev->flags |= TI_EVENT_FLAG_SAVE;

    return 0;
}

/*
 * Returns 0 on success
 * - for example: {...}
 */
static int rjob__new_procedure(mp_unp_t * up)
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
                "job `new_procedure` for `.thingsdb`: "
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
                "job `new_procedure` for `.thingsdb`: "
                "procedure `%s` already exists",
                procedure->name);

failed:
    ti_procedure_destroy(procedure);
    ti_val_drop((ti_val_t *) closure);
    return -1;
}

/*
 * Returns 0 on success
 * - for example: '{...}'
 */
static int rjob__new_timer(mp_unp_t * up)
{
    mp_obj_t obj, mp_id, mp_next_run, mp_repeat, mp_user_id;
    ti_timer_t * timer = NULL;
    ti_varr_t * varr = NULL;
    ti_closure_t * closure;
    ti_user_t * user;
    ti_vup_t vup = {
            .isclient = false,
            .collection = NULL,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 6 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_next_run) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_repeat) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_user_id) != MP_U64 ||
        mp_skip(up) != MP_STR)
    {
        log_critical("job `new_timer` for the @thingsdb scope: invalid data");
        return -1;
    }

    user = ti_users_get_by_id(mp_user_id.via.u64);
    closure = (ti_closure_t *) ti_val_from_vup(&vup);

    if (!closure || !ti_val_is_closure((ti_val_t *) closure) ||
        mp_skip(up) != MP_STR ||
        vec_reserve(&ti.timers->timers, 1))
        goto fail0;

    varr = (ti_varr_t *) ti_val_from_vup(&vup);
    if (!varr || !ti_val_is_array((ti_val_t *) varr))
        goto fail0;

    timer = ti_timer_create(
                mp_id.via.u64,
                mp_next_run.via.u64,
                mp_repeat.via.u64,
                TI_SCOPE_THINGSDB,
                user,
                closure,
                varr->vec);
    if (!timer)
        goto fail0;

    ti_update_next_thing_id(timer->id);
    VEC_push(ti.timers->timers, timer);
    free(varr);
    ti_decref(closure);
    return 0;

fail0:
    log_critical("job `new_timer` for the @thingsdb scope has failed");
    ti_val_drop((ti_val_t *) varr);
    ti_val_drop((ti_val_t *) closure);
    return -1;
}

/*
 * Returns 0 on success
 * - for example: {'id': id, 'key': value}, 'expire_ts': ts, 'description':..}
 */
static int rjob__new_token(mp_unp_t * up)
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
        log_critical("job `new_token`: invalid format");
        return -1;
    }

    if (mp_key.via.str.n != sizeof(ti_token_key_t))
    {
        log_critical("job `new_token`: invalid key size");
        return -1;
    }

    user = ti_users_get_by_id(mp_user.via.u64);
    if (!user)
    {
        log_critical(
                "job `new_token`: "TI_USER_ID" not found",
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
static int rjob__new_user(mp_unp_t * up)
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
        log_critical("job `new_user`: invalid format");
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
        log_critical("job `new_user`: %s", e.msg);
        return -1;
    }

    return 0;
}

/*
 * Returns 0 on success
 * - for example: id
 */
static int rjob__del_node(ti_event_t * ev, mp_unp_t * up)
{
    ti_node_t * this_node = ti.node;
    mp_obj_t mp_node;

    if (mp_next(up, &mp_node) != MP_U64)
    {
        log_critical("job `del_node`: invalid format");
        return -1;
    }

    if (ev->id <= ti.last_event_id)
        return 0;   /* this job is already applied */

    if (mp_node.via.u64 == this_node->id)
    {
        log_critical("cannot delete node with id %"PRIu64, mp_node.via.u64);
        return -1;
    }

    ti_nodes_del_node(mp_node.via.u64);

    ev->flags |= TI_EVENT_FLAG_SAVE;

    return 0;
}

/*
 * Returns 0 on success
 * - for example: module_name
 */
static int rjob__del_module(mp_unp_t * up)
{
    ti_module_t * module;
    mp_obj_t mp_name;

    if (mp_next(up, &mp_name) != MP_STR)
    {
        log_critical("job `del_module`: invalid format");
        return -1;
    }

    module = ti_modules_by_strn(mp_name.via.str.data, mp_name.via.str.n);
    if (!module)
    {
        log_error("job `del_module`: module `%.*s` not found",
                mp_name.via.str.n,
                mp_name.via.str.data);
        return 0;  /* error, but able to continue */
    }

    ti_module_del(module);
    return 0;
}

/*
 * Returns 0 on success
 * - for example: {"name": module_name, "scope_id": nil/id}
 */
static int rjob__set_module_scope(mp_unp_t * up)
{
    ti_module_t * module;
    mp_obj_t obj, mp_name, mp_scope;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_skip(up) != MP_STR ||
        (mp_next(up, &mp_scope) != MP_U64 && mp_scope.tp != MP_NIL))
    {
        log_critical("job `set_module_scope`: invalid format");
        return -1;
    }

    module = ti_modules_by_strn(mp_name.via.str.data, mp_name.via.str.n);
    if (!module)
    {
        log_error("job `set_module_scope`: module `%.*s` not found",
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
 * - for example: '{...}'
 */
static int rjob__set_timer_args(mp_unp_t * up)
{
    mp_obj_t obj, mp_id;
    ti_varr_t * varr;
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
                "job `set_timer_args` for the @thingsdb scope has failed: "
                "invalid data");
        return -1;
    }

    varr = (ti_varr_t *) ti_val_from_vup(&vup);
    if (!varr || !ti_val_is_array((ti_val_t *) varr))
        goto fail0;

    for (vec_each(ti.timers->timers, ti_timer_t, timer))
    {
        if (timer->id == mp_id.via.u64)
        {
            vec_t * tmp = timer->args;
            timer->args = varr->vec;
            varr->vec = tmp;
            break;
        }
    }

    /* it may happen that the timer is already gone, this is not an issue */
    ti_val_drop((ti_val_t *) varr);
    return 0;

fail0:
    log_critical("job `set_timer_args` for the @thingsdb scope has failed");
    ti_val_drop((ti_val_t *) varr);
    return -1;
}

/*
 * Returns 0 on success
 * - for example: {"name": module_name, "conf_pkg": nil/bin}
 */
static int rjob__set_module_conf(mp_unp_t * up)
{
    ti_module_t * module;
    mp_obj_t obj, mp_name, mp_pkg;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_skip(up) != MP_STR ||
        (mp_next(up, &mp_pkg) != MP_BIN && mp_pkg.tp != MP_NIL))
    {
        log_critical("job `set_module_conf`: invalid format");
        return -1;
    }

    module = ti_modules_by_strn(mp_name.via.str.data, mp_name.via.str.n);
    if (!module)
    {
        log_error("job `set_module_conf`: module `%.*s` not found",
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
static int rjob__rename_collection(mp_unp_t * up)
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
        log_critical("job `rename_collection`: invalid format");
        return -1;
    }

    collection = ti_collections_get_by_id(mp_id.via.u64);
    if (!collection)
    {
        log_critical(
                "job `rename_collection`: "TI_COLLECTION_ID" not found",
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
static int rjob__set_time_zone(mp_unp_t * up)
{
    ti_collection_t * collection;
    mp_obj_t obj, mp_id, mp_tz;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_tz) != MP_U64)
    {
        log_critical("job `set_time_zone`: invalid format");
        return -1;
    }

    collection = ti_collections_get_by_id(mp_id.via.u64);
    if (!collection)
    {
        log_critical(
                "job `set_time_zone`: "TI_COLLECTION_ID" not found",
                mp_id.via.u64);
        return -1;
    }

    collection->tz = ti_tz_from_index(mp_tz.via.u64);
    return 0;
}

/*
 * Returns 0 on success
 * - for example: {'old':name, 'name':name}
 */
static int rjob__rename_module(mp_unp_t * up)
{
    ti_module_t * module;
    mp_obj_t obj, mp_old, mp_name;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_old) != MP_STR ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR)
    {
        log_critical("job `rename_module`: invalid format");
        return -1;
    }

    module = ti_modules_by_strn(mp_old.via.str.data, mp_old.via.str.n);

    if (!module)
    {
        log_critical(
                "job `rename_module` cannot find `%.*s`",
                mp_old.via.str.n, mp_old.via.str.data);
        return -1;
    }

    return ti_modules_rename(module, mp_name.via.str.data, mp_name.via.str.n);
}

/*
 * Returns 0 on success
 * - for example: {'old':name, 'name':name}
 */
static int rjob__rename_procedure(mp_unp_t * up)
{
    ti_procedure_t * procedure;
    mp_obj_t obj, mp_old, mp_name;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_old) != MP_STR ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR)
    {
        log_critical("job `rename_procedure`: invalid format");
        return -1;
    }

    procedure = ti_procedures_by_strn(
            ti.procedures,
            mp_old.via.str.data,
            mp_old.via.str.n);

    if (!procedure)
    {
        log_critical(
                "job `rename_procedure` cannot find `%.*s`",
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
static int rjob__rename_user(mp_unp_t * up)
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
        log_critical("job `rename_user`: invalid format");
        return -1;
    }

    user = ti_users_get_by_id(mp_id.via.u64);
    if (!user)
    {
        log_critical(
                "job `rename_user`: "TI_USER_ID" not found", mp_id.via.u64);
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
static int rjob__restore(mp_unp_t * up)
{
    mp_obj_t obj;

    switch (mp_next(up, &obj))
    {
    case MP_BOOL:
        break;
    default:
        log_critical("job `restore`: invalid format");
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
static int rjob__revoke(mp_unp_t * up)
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
        log_critical("job `revoke`: invalid format");
        return -1;
    }

    if (mp_scope.via.u64 > 1)
    {
        collection = ti_collections_get_by_id(mp_scope.via.u64);
        if (!collection)
        {
            log_critical(
                    "job `revoke`: "TI_COLLECTION_ID" not found",
                    mp_scope.via.u64);
            return -1;
        }
    }

    user = ti_users_get_by_id(mp_user.via.u64);
    if (!user)
    {
        log_critical("job `revoke`: "TI_USER_ID" not found", mp_user.via.u64);
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
static int rjob__set_password(mp_unp_t * up)
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
        log_critical("job `set_password`: invalid format");
        return -1;
    }

    user = ti_users_get_by_id(mp_user.via.u64);
    if (!user)
    {
        log_critical(
                "job `set_password`: "TI_USER_ID" not found",
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

int ti_rjob_run(ti_event_t * ev, mp_unp_t * up)
{
    mp_obj_t obj, mp_job;
    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_next(up, &mp_job) != MP_STR || mp_job.via.str.n < 2)
    {
        log_critical(
                "job is not a `map` or `type` "
                "for thing "TI_THING_ID" is missing", 0);
        return -1;
    }

    switch (*mp_job.via.str.data)
    {
    case 'c':
        if (mp_str_eq(&mp_job, "clear_users"))
            return rjob__clear_users(up);
        break;
    case 'd':
        if (mp_str_eq(&mp_job, "del_timer"))
            return rjob__del_timer(up);
        if (mp_str_eq(&mp_job, "del_collection"))
            return rjob__del_collection(up);
        if (mp_str_eq(&mp_job, "del_expired"))
            return rjob__del_expired(up);
        if (mp_str_eq(&mp_job, "del_node"))
            return rjob__del_node(ev, up);
        if (mp_str_eq(&mp_job, "del_module"))
            return rjob__del_module(up);
        if (mp_str_eq(&mp_job, "del_procedure"))
            return rjob__del_procedure(up);
        if (mp_str_eq(&mp_job, "del_token"))
            return rjob__del_token(up);
        if (mp_str_eq(&mp_job, "del_user"))
            return rjob__del_user(up);
        break;
    case 'g':
        if (mp_str_eq(&mp_job, "grant"))
            return rjob__grant(up);
        break;
    case 'n':
        if (mp_str_eq(&mp_job, "new_timer"))
            return rjob__new_timer(up);
        if (mp_str_eq(&mp_job, "new_collection"))
            return rjob__new_collection(up);
        if (mp_str_eq(&mp_job, "new_module"))
            return rjob__new_module(up);
        if (mp_str_eq(&mp_job, "new_node"))
            return rjob__new_node(ev, up);
        if (mp_str_eq(&mp_job, "new_procedure"))
            return rjob__new_procedure(up);
        if (mp_str_eq(&mp_job, "new_token"))
            return rjob__new_token(up);
        if (mp_str_eq(&mp_job, "new_user"))
            return rjob__new_user(up);
        break;
    case 'r':
        if (mp_str_eq(&mp_job, "rename_collection"))
            return rjob__rename_collection(up);
        if (mp_str_eq(&mp_job, "rename_module"))
            return rjob__rename_module(up);
        if (mp_str_eq(&mp_job, "rename_procedure"))
            return rjob__rename_procedure(up);
        if (mp_str_eq(&mp_job, "rename_user"))
            return rjob__rename_user(up);
        if (mp_str_eq(&mp_job, "restore"))
            return rjob__restore(up);
        if (mp_str_eq(&mp_job, "revoke"))
            return rjob__revoke(up);
        break;
    case 's':
        if (mp_str_eq(&mp_job, "set_timer_args"))
            return rjob__set_timer_args(up);
        if (mp_str_eq(&mp_job, "set_module_conf"))
            return rjob__set_module_conf(up);
        if (mp_str_eq(&mp_job, "set_module_scope"))
            return rjob__set_module_scope(up);
        if (mp_str_eq(&mp_job, "set_password"))
            return rjob__set_password(up);
        if (mp_str_eq(&mp_job, "set_time_zone"))
            return rjob__set_time_zone(up);
        if (mp_str_eq(&mp_job, "set_quota"))
        {
            /*
             * DEPRECATED: `set_quota` is removed, skip this job
             */
            mp_skip(up);
            return 0;
        }
        break;
    case 't':
        if (mp_str_eq(&mp_job, "take_access"))
            return rjob__take_access(up);
        break;
    }

    log_critical("unknown job: `%.*s`", mp_job.via.str.n, mp_job.via.str.data);
    return -1;
}
