/*
 * ti/users.h
 */
#ifndef TI_USERS_H_
#define TI_USERS_H_

#include <ex.h>
#include <stdint.h>
#include <ti/raw.h>
#include <ti/user.h>
#include <ti/val.h>
#include <util/mpack.h>
#include <util/vec.h>

int ti_users_create(void);
void ti_users_destroy(void);
ti_user_t * ti_users_new_user(
        const char * name,
        size_t name_n,
        const char * passstr,
        uint64_t created_at,
        ex_t * e);
ti_user_t * ti_users_load_user(
        uint64_t user_id,
        const char * name,
        size_t name_n,
        const char * encrypted,
        uint64_t created_at,
        ex_t * e);
int ti_users_clear(void);
void ti_users_del_user(ti_user_t * user);
ti_user_t * ti_users_auth(mp_obj_t * mp_name, mp_obj_t * mp_pass, ex_t * e);
ti_user_t * ti_users_auth_by_token(mp_obj_t * mp_token, ex_t * e);
ti_user_t * ti_users_auth_by_basic(const char * b64, size_t n, ex_t * e);
ti_user_t * ti_users_get_by_id(uint64_t id);
ti_user_t * ti_users_get_by_namestrn(const char * name, size_t n);
ti_varr_t * ti_users_info(void);
void ti_users_del_expired(uint64_t after_ts);
_Bool ti_users_has_token(ti_token_key_t * key);
ti_token_t * ti_users_pop_token_by_key(ti_token_key_t * key);

#endif /* TI_USERS_H_ */
