/*
 * users.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef TI_USERS_H_
#define TI_USERS_H_

#include <stdint.h>
#include <qpack.h>
#include <ti/raw.h>
#include <ti/user.h>
#include <util/vec.h>
#include <util/ex.h>

ti_user_t * ti_users_auth(
        vec_t * users,
        qp_obj_t * name,
        qp_obj_t * pass,
        ex_t * e);
ti_user_t * ti_users_get_by_id(const vec_t * users, uint64_t id);
ti_user_t * ti_users_get_by_name(const vec_t * users, ti_raw_t * name);
int ti_users_store(const vec_t * users, const char * fn);
int ti_users_restore(vec_t ** users, const char * fn);
#endif /* TI_USERS_H_ */
