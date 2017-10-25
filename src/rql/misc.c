/*
 * misc.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <rql/misc.h>
#include <rql/api.h>
#include <rql/rql.h>
#include <rql/task.h>
#include <rql/auth.h>

/*
 * Pack:
 *
 * {
 *   '_': [{
 *       '_t': 0,
 *       '_u': "iris",
 (       '_p': "siri"
 *   }]
 * }
 */
qp_packer_t * rql_misc_pack_init_event_request(void)
{
    qp_packer_t * packer = qp_packer_create(40);  /* more than enough */
    if (!packer) return NULL;

    if (qp_add_map(&packer) ||
        qp_add_raw_from_str(packer, rql_name) ||
        qp_add_array(&packer) ||
        qp_add_map(&packer) ||

        qp_add_raw_from_str(packer, RQL_API_TASK) ||
        qp_add_int64(packer, RQL_TASK_USER_CREATE) ||
        qp_add_raw_from_str(packer, RQL_API_USER) ||
        qp_add_raw_from_str(packer, rql_user_def_name) ||
        qp_add_raw_from_str(packer, RQL_API_PASS) ||
        qp_add_raw_from_str(packer, rql_user_def_pass) ||
        qp_close_map(packer) ||

        qp_add_map(&packer) ||
        qp_add_raw_from_str(packer, RQL_API_TASK) ||
        qp_add_int64(packer, RQL_TASK_GRANT) ||
        qp_add_raw_from_str(packer, RQL_API_USER) ||
        qp_add_raw_from_str(packer, rql_user_def_name) ||
        qp_add_raw_from_str(packer, RQL_API_PERM) ||
        qp_add_int64(packer, RQL_AUTH_MASK_FULL) ||
        qp_close_map(packer) ||

        qp_close_array(packer) ||
        qp_close_map(packer))
    {
        qp_packer_destroy(packer);
        return NULL;
    }

    return packer;
}
