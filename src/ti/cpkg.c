/*
 * ti/cpkg.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/auth.h>
#include <ti/cpkg.h>
#include <ti/proto.h>
#include <ti/task.t.h>
#include <util/mpack.h>
#include <util/cryptx.h>

ti_cpkg_t * ti_cpkg_create(ti_pkg_t * pkg, uint64_t change_id)
{
    ti_cpkg_t * cpkg = malloc(sizeof(ti_cpkg_t));
    if (!cpkg)
        return NULL;
    cpkg->ref = 1;
    cpkg->pkg = pkg;
    cpkg->flags = 0;
    cpkg->change_id = change_id;
    return cpkg;
}

ti_cpkg_t * ti_cpkg_initial(void)
{
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    const uint64_t change_id = 1;
    const uint64_t thing_id = 0;               /* parent root thing */
    uint64_t user_id = ti_next_free_id();      /* id:1 */
    uint64_t stuff_id = ti_next_free_id();     /* id:2 !important: id > 1 */
    ti_cpkg_t * cpkg;
    ti_pkg_t * pkg;
    char salt[CRYPTX_SALT_SZ];
    char encrypted[CRYPTX_SZ];

    assert (user_id == 1);  /* first user must have id 1, see restore */

    /* collection id must be >1, see TI_SCOPE_THINGSDB and TI_SCOPE_NODE */
    assert (stuff_id > 1);

    /* generate a random salt */
    cryptx_gen_salt(salt);

    /* encrypt the users password */
    cryptx(ti_user_def_pass, salt, encrypted);

    if (mp_sbuffer_alloc_init(&buffer, 1024, sizeof(ti_pkg_t)))
        return NULL;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2+1);

    msgpack_pack_uint64(&pk, change_id);
    msgpack_pack_uint64(&pk, TI_SCOPE_THINGSDB);

    msgpack_pack_array(&pk, 1+5);

    msgpack_pack_uint64(&pk, thing_id);

    msgpack_pack_array(&pk, 2);           /* task 1 */

    msgpack_pack_uint8(&pk, TI_TASK_NEW_USER);
    msgpack_pack_map(&pk, 3);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, user_id);

    mp_pack_str(&pk, "username");
    mp_pack_str(&pk, ti_user_def_name);

    mp_pack_str(&pk, "created_at");
    msgpack_pack_uint64(&pk, util_now_usec());

    msgpack_pack_array(&pk, 2);           /* task 2 */

    msgpack_pack_uint8(&pk, TI_TASK_SET_PASSWORD);
    msgpack_pack_map(&pk, 2);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, user_id);

    mp_pack_str(&pk, "password");
    mp_pack_str(&pk, encrypted);

    msgpack_pack_array(&pk, 2);           /* task 3 */

    msgpack_pack_uint8(&pk, TI_TASK_GRANT);
    msgpack_pack_map(&pk, 3);

    mp_pack_str(&pk, "scope");
    msgpack_pack_uint64(&pk, TI_SCOPE_NODE);

    mp_pack_str(&pk, "user");
    msgpack_pack_uint64(&pk, user_id);

    mp_pack_str(&pk, "mask");
    msgpack_pack_uint64(&pk, TI_AUTH_MASK_FULL);

    msgpack_pack_array(&pk, 2);           /* task 4 */

    msgpack_pack_uint8(&pk, TI_TASK_GRANT);
    msgpack_pack_map(&pk, 3);

    mp_pack_str(&pk, "scope");
    msgpack_pack_uint64(&pk, TI_SCOPE_THINGSDB);

    mp_pack_str(&pk, "user");
    msgpack_pack_uint64(&pk, user_id);

    mp_pack_str(&pk, "mask");
    msgpack_pack_uint64(&pk, TI_AUTH_MASK_FULL);

    msgpack_pack_array(&pk, 2);           /* task 5 */

    msgpack_pack_uint8(&pk, TI_TASK_NEW_COLLECTION);
    msgpack_pack_map(&pk, 4);

    mp_pack_str(&pk, "name");
    mp_pack_str(&pk, "stuff");

    mp_pack_str(&pk, "user");
    msgpack_pack_uint64(&pk, user_id);

    mp_pack_str(&pk, "root");
    msgpack_pack_uint64(&pk, stuff_id);

    mp_pack_str(&pk, "created_at");
    msgpack_pack_uint64(&pk, util_now_usec());

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_NODE_CHANGE, buffer.size);

    cpkg = ti_cpkg_create(pkg, change_id);
    if (!cpkg)
    {
        free(pkg);
        return NULL;
    }
    return cpkg;
}

ti_cpkg_t * ti_cpkg_from_pkg(ti_pkg_t * pkg)
{
    ti_cpkg_t * cpkg;
    mp_unp_t up;
    mp_obj_t obj, mp_change_id;

    pkg = ti_pkg_dup(pkg);
    if (!pkg)
    {
        log_critical(EX_MEMORY_S);
        return NULL;
    }

    mp_unp_init(&up, pkg->data, pkg->n);

    if (mp_next(&up, &obj) != MP_ARR || !obj.via.sz ||
        mp_next(&up, &mp_change_id) != MP_U64)
    {
        /*
         * TODO (COMPAT) For compatibility with v0.x
         */
        if (obj.tp != MP_MAP || !obj.via.sz ||
            mp_next(&up, &obj) != MP_ARR || !obj.via.sz ||
            mp_next(&up, &mp_change_id) != MP_U64)
        {
            log_error("invalid package");
            return NULL;
        }
    }
    cpkg = ti_cpkg_create(pkg, mp_change_id.via.u64);
    if (!cpkg)
    {
        free(pkg);
        log_critical(EX_MEMORY_S);
    }

    return cpkg;
}
