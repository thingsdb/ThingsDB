/*
 * ti/user.h
 */
#ifndef TI_USER_H_
#define TI_USER_H_

typedef struct ti_user_s  ti_user_t;

#include <stdint.h>
#include <ti/raw.h>
#include <ti/val.h>
#include <ti/ex.h>
#include <ti/token.h>
#include <util/vec.h>

extern const char * ti_user_def_name;
extern const char * ti_user_def_pass;
extern const unsigned int ti_min_name;
extern const unsigned int ti_max_name;
extern const unsigned int ti_min_pass;
extern const unsigned int ti_max_pass;

ti_user_t * ti_user_create(
        uint64_t id,
        const char * name,
        size_t n,
        const char * encrpass);
void ti_user_drop(ti_user_t * user);
static inline int ti_user_add_token(ti_user_t * user, ti_token_t * token);
void ti_user_del_expired(ti_user_t * user, uint64_t after_ts);
_Bool ti_user_name_check(const char * name, size_t n, ex_t * e);
_Bool ti_user_pass_check(const char * passstr, ex_t * e);
int ti_user_rename(ti_user_t * user, ti_raw_t * name, ex_t * e);
int ti_user_set_pass(ti_user_t * user, const char * pass);
int ti_user_to_packer(ti_user_t * user, qp_packer_t ** packer);
ti_val_t * ti_user_as_qpval(ti_user_t * user);
ti_token_t * ti_user_del_token_by_key(ti_user_t * user, ti_token_key_t * key);

struct ti_user_s
{
    uint32_t ref;
    uint64_t id;
    ti_raw_t * name;
    char * encpass;     /* may be NULL if no password is set */
    vec_t * tokens;     /* ti_token_t */
};

static inline int ti_user_add_token(ti_user_t * user, ti_token_t * token)
{
    return vec_push(&user->tokens, token);
}

#endif /* TI_USER_H_ */
