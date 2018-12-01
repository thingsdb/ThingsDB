/*
 * misc.c
 */
#include <ti/auth.h>
#include <ti/misc.h>

qp_packer_t * ti_misc_init_query(void)
{
    qp_packer_t * packer = qp_packer_create2(128, 5);
    if (!packer)
        return NULL;

    (void) qp_add_map(&packer);

    (void) qp_add_array(&packer);
    (void) qp_add_int64(packer, 1);
    (void) qp_add_int64(packer, 0);  /* target 0 */
    (void) qp_close_array(packer);

    (void) qp_add_map(&packer);

    (void) qp_add_int64(packer, 0);  /* thing 0 */
    (void) qp_add_array(&packer);

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "user_new");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "username");
    (void) qp_add_raw_from_str(packer, ti_user_def_name);
    (void) qp_add_raw_from_str(packer, "password");
    (void) qp_add_raw_from_str(packer, ti_user_def_pass);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "grant");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "target");
    (void) qp_add_int64(packer, 0);
    (void) qp_add_raw_from_str(packer, "user");
    (void) qp_add_raw_from_str(packer, ti_user_def_name);
    (void) qp_add_raw_from_str(packer, "flags");
    (void) qp_add_int64(packer, TI_AUTH_FULL);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    (void) qp_close_array(packer);

    (void) qp_close_map(packer);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    return packer;
}
