/*
 * misc.c
 */
#include <ti/auth.h>
#include <ti/misc.h>

qp_packer_t * ti_misc_init_query(void)
{
    qp_packer_t * packer = qp_packer_create(128);  /* more than enough */
    if (!packer)
        return NULL;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "query");
    (void) qp_add_raw_from_fmt(
                packer,
                "users.create(%s).password='%s';grant(%s,%u)",
                ti_user_def_name,
                ti_user_def_pass,
                ti_user_def_name,
                TI_AUTH_MASK_FULL);
    (void) qp_close_map(packer);

    return packer;
}
