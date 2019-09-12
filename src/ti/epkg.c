/*
 * ti/epkg.c
 */
#include <assert.h>
#include <ti/epkg.h>
#include <ti/auth.h>
#include <ti/proto.h>
#include <ti.h>
#include <stdlib.h>
#include <util/qpx.h>
#include <util/cryptx.h>

ti_epkg_t * ti_epkg_create(ti_pkg_t * pkg, uint64_t event_id)
{
    ti_epkg_t * epkg = malloc(sizeof(ti_epkg_t));
    if (!epkg)
        return NULL;
    epkg->ref = 1;
    epkg->pkg = pkg;
    epkg->event_id = event_id;
    return epkg;
}

ti_epkg_t * ti_epkg_initial(void)
{
    uint64_t event_id = 1;
    uint64_t scope_id = 0;
    uint64_t thing_id = 0;                      /* parent root thing */
    uint64_t user_id = ti_next_thing_id();      /* id:1 */
    uint64_t stuff_id = ti_next_thing_id();     /* id:2 !important: id > 1 */
    ti_epkg_t * epkg;
    ti_pkg_t * pkg;
    qpx_packer_t * packer;
    char salt[CRYPTX_SALT_SZ];
    char encrypted[CRYPTX_SZ];

    /* collection id must be >1, see TI_SCOPE_THINGSDB and TI_SCOPE_NODE */
    assert (stuff_id > 1);

    /* generate a random salt */
    cryptx_gen_salt(salt);

    /* encrypt the users password */
    cryptx(ti_user_def_pass, salt, encrypted);

    packer = qpx_packer_create(1024, 5);
    if (!packer)
        return NULL;

    (void) qp_add_map(&packer);

    (void) qp_add_array(&packer);
    (void) qp_add_int(packer, event_id);
    (void) qp_add_int(packer, scope_id);
    (void) qp_close_array(packer);

    (void) qp_add_map(&packer);

    (void) qp_add_int(packer, thing_id);
    (void) qp_add_array(&packer);

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "new_user");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "id");
    (void) qp_add_int(packer, user_id);
    (void) qp_add_raw_from_str(packer, "username");
    (void) qp_add_raw_from_str(packer, ti_user_def_name);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "set_password");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "id");
    (void) qp_add_int(packer, user_id);
    (void) qp_add_raw_from_str(packer, "password");
    (void) qp_add_raw_from_str(packer, encrypted);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "grant");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "target");
    (void) qp_add_int(packer, TI_SCOPE_NODE);
    (void) qp_add_raw_from_str(packer, "user");
    (void) qp_add_int(packer, user_id);
    (void) qp_add_raw_from_str(packer, "mask");
    (void) qp_add_int(packer, TI_AUTH_MASK_FULL);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "grant");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "target");
    (void) qp_add_int(packer, TI_SCOPE_THINGSDB);
    (void) qp_add_raw_from_str(packer, "user");
    (void) qp_add_int(packer, user_id);
    (void) qp_add_raw_from_str(packer, "mask");
    (void) qp_add_int(packer, TI_AUTH_MASK_FULL);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "new_collection");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "name");
    (void) qp_add_raw_from_str(packer, "stuff");
    (void) qp_add_raw_from_str(packer, "user");
    (void) qp_add_int(packer, user_id);
    (void) qp_add_raw_from_str(packer, "root");
    (void) qp_add_int(packer, stuff_id);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    (void) qp_close_array(packer);

    (void) qp_close_map(packer);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    pkg = qpx_packer_pkg(packer, TI_PROTO_NODE_EVENT);
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
    qp_unpacker_t unpacker;
    qp_obj_t qp_event_id;
    uint64_t event_id;

    pkg = ti_pkg_dup(pkg);
    if (!pkg)
    {
        log_critical(EX_MEMORY_S);
        return NULL;
    }

    qp_unpacker_init(&unpacker, pkg->data, pkg->n);

    if (!qp_is_map(qp_next(&unpacker, NULL)) ||
        !qp_is_array(qp_next(&unpacker, NULL)) ||
        !qp_is_int(qp_next(&unpacker, &qp_event_id)))
    {
        log_error("invalid package");
        return NULL;
    }

    event_id = (uint64_t) qp_event_id.via.int64;

    epkg = ti_epkg_create(pkg, event_id);
    if (!epkg)
    {
        free(pkg);
        log_critical(EX_MEMORY_S);
    }

    return epkg;
}
