/*
 * ti/users.h
 */
#ifndef TI_USERS_H_
#define TI_USERS_H_

typedef struct ti_users_s ti_users_t;

#include <stdint.h>
#include <qpack.h>
#include <ti/ex.h>
#include <ti/raw.h>
#include <ti/user.h>
#include <ti/val.h>

int ti_users_create(void);
void ti_users_destroy(void);
ti_user_t * ti_users_new_user(
        const char * name,
        size_t n,
        const char * passstr,
        ex_t * e);
ti_user_t * ti_users_load_user(
        uint64_t user_id,
        const char * name,
        size_t n,
        const char * encrypted,
        ex_t * e);
int ti_users_clear(void);
void ti_users_del_user(ti_user_t * user);
ti_user_t * ti_users_auth(qp_obj_t * name, qp_obj_t * pass, ex_t * e);
ti_user_t * ti_users_get_by_id(uint64_t id);
ti_user_t * ti_users_get_by_namestrn(const char * name, size_t n);
ti_val_t * ti_users_as_qpval(void);


struct ti_users_s
{
    vec_t * vec;        /* ti_user_t with reference */
};

#endif /* TI_USERS_H_ */
