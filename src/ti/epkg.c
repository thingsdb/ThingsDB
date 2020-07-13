/*
 * ti/epkg.c
 */
#include <assert.h>
#include <ti/epkg.h>
#include <ti/auth.h>
#include <ti/proto.h>
#include <ti.h>
#include <stdlib.h>
#include <util/mpack.h>
#include <util/cryptx.h>

ti_epkg_t * ti_epkg_create(ti_pkg_t * pkg, uint64_t event_id)
{
    ti_epkg_t * epkg = malloc(sizeof(ti_epkg_t));
    if (!epkg)
        return NULL;
    epkg->ref = 1;
    epkg->pkg = pkg;
    epkg->flags = 0;
    epkg->event_id = event_id;
    return epkg;
}

ti_epkg_t * ti_epkg_initial(void)
{
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    uint64_t event_id = 1;
    uint64_t scope_id = 0;                      /* TI_SCOPE_THINGSDB */
    uint64_t thing_id = 0;                      /* parent root thing */
    uint64_t user_id = ti_next_thing_id();      /* id:1 */
    uint64_t stuff_id = ti_next_thing_id();     /* id:2 !important: id > 1 */
    ti_epkg_t * epkg;
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

    msgpack_pack_map(&pk, 1);

    msgpack_pack_array(&pk, 2);
    msgpack_pack_uint64(&pk, event_id);
    msgpack_pack_uint64(&pk, scope_id);

    msgpack_pack_map(&pk, 1);

    msgpack_pack_uint64(&pk, thing_id);
    msgpack_pack_array(&pk, 5);

    msgpack_pack_map(&pk, 1);           /* job 1 */

    mp_pack_str(&pk, "new_user");
    msgpack_pack_map(&pk, 3);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, user_id);

    mp_pack_str(&pk, "username");
    mp_pack_str(&pk, ti_user_def_name);

    mp_pack_str(&pk, "created_at");
    msgpack_pack_uint64(&pk, util_now_tsec());

    msgpack_pack_map(&pk, 1);           /* job 2 */

    mp_pack_str(&pk, "set_password");
    msgpack_pack_map(&pk, 2);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, user_id);

    mp_pack_str(&pk, "password");
    mp_pack_str(&pk, encrypted);

    msgpack_pack_map(&pk, 1);           /* job 3 */

    mp_pack_str(&pk, "grant");
    msgpack_pack_map(&pk, 3);

    mp_pack_str(&pk, "scope");
    msgpack_pack_uint64(&pk, TI_SCOPE_NODE);

    mp_pack_str(&pk, "user");
    msgpack_pack_uint64(&pk, user_id);

    mp_pack_str(&pk, "mask");
    msgpack_pack_uint64(&pk, TI_AUTH_MASK_FULL);

    msgpack_pack_map(&pk, 1);           /* job 4 */

    mp_pack_str(&pk, "grant");
    msgpack_pack_map(&pk, 3);

    mp_pack_str(&pk, "scope");
    msgpack_pack_uint64(&pk, TI_SCOPE_THINGSDB);

    mp_pack_str(&pk, "user");
    msgpack_pack_uint64(&pk, user_id);

    mp_pack_str(&pk, "mask");
    msgpack_pack_uint64(&pk, TI_AUTH_MASK_FULL);

    msgpack_pack_map(&pk, 1);           /* job 5 */

    mp_pack_str(&pk, "new_collection");
    msgpack_pack_map(&pk, 4);

    mp_pack_str(&pk, "name");
    mp_pack_str(&pk, "stuff");

    mp_pack_str(&pk, "user");
    msgpack_pack_uint64(&pk, user_id);

    mp_pack_str(&pk, "root");
    msgpack_pack_uint64(&pk, stuff_id);

    mp_pack_str(&pk, "created_at");
    msgpack_pack_uint64(&pk, util_now_tsec());

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_NODE_EVENT, buffer.size);

    epkg = ti_epkg_create(pkg, event_id);
    if (!epkg)
    {
        free(pkg);
        return NULL;
    }
    return epkg;
}

ti_epkg_t * ti_epkg_from_pkg(ti_pkg_t * pkg)
{
    ti_epkg_t * epkg;
    mp_unp_t up;
    mp_obj_t obj, mp_event_id;

    pkg = ti_pkg_dup(pkg);
    if (!pkg)
    {
        log_critical(EX_MEMORY_S);
        return NULL;
    }

    mp_unp_init(&up, pkg->data, pkg->n);

    if (mp_next(&up, &obj) != MP_MAP || !obj.via.sz ||
        mp_next(&up, &obj) != MP_ARR || !obj.via.sz ||
        mp_next(&up, &mp_event_id) != MP_U64)
    {
        log_error("invalid package");
        return NULL;
    }

    epkg = ti_epkg_create(pkg, mp_event_id.via.u64);
    if (!epkg)
    {
        free(pkg);
        log_critical(EX_MEMORY_S);
    }

    return epkg;
}
