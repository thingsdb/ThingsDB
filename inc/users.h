/*
 * users.h
 */
#ifndef THINGSDB_USERS_H_
#define THINGSDB_USERS_H_

#include <stdint.h>
#include <qpack.h>
#include <ti/raw.h>
#include <ti/user.h>
#include <util/ex.h>

int thingsdb_users_create(void);
void thingsdb_users_destroy(void);
ti_user_t * thingsdb_users_auth(qp_obj_t * name, qp_obj_t * pass, ex_t * e);
ti_user_t * thingsdb_users_get_by_id(uint64_t id);
ti_user_t * thingsdb_users_get_by_name(ti_raw_t * name);
int thingsdb_users_store( const char * fn);
int thingsdb_users_restore(const char * fn);
#endif /* THINGSDB_USERS_H_ */
