/*
 * users.h
 */
#ifndef TI_USERS_H_
#define TI_USERS_H_

#include <stdint.h>
#include <qpack.h>
#include <ti/ex.h>
#include <ti/raw.h>
#include <ti/user.h>

int ti_users_create(void);
void ti_users_destroy(void);
ti_user_t * ti_users_auth(qp_obj_t * name, qp_obj_t * pass, ex_t * e);
ti_user_t * ti_users_get_by_id(uint64_t id);
ti_user_t * ti_users_get_by_name(ti_raw_t * name);
int ti_users_store( const char * fn);
int ti_users_restore(const char * fn);
#endif /* TI_USERS_H_ */
