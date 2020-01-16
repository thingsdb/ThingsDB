/*
 * ti/user.h
 */
#ifndef TI_USER_H_
#define TI_USER_H_

typedef struct ti_user_s  ti_user_t;

#include <ex.h>
#include <stdint.h>
#include <ti/raw.h>
#include <ti/token.h>
#include <ti/val.h>
#include <util/mpack.h>
#include <util/vec.h>

extern const char * ti_user_def_name;
extern const char * ti_user_def_pass;
extern const unsigned int ti_min_name;
extern const unsigned int ti_max_name;
extern const unsigned int ti_min_pass;
extern const unsigned int ti_max_pass;

ti_user_t* ti_user_create(
        uint64_t id,
        const char * name,
        size_t name_n,
        const char * encrpass,
        uint64_t created_at);
void ti_user_drop(ti_user_t * user);
static inline int ti_user_add_token(ti_user_t * user, ti_token_t * token);
void ti_user_del_expired(ti_user_t * user, uint64_t after_ts);
_Bool ti_user_name_check(const char * name, size_t n, ex_t * e);
_Bool ti_user_pass_check(const char * passstr, ex_t * e);
int ti_user_rename(ti_user_t * user, ti_raw_t * name, ex_t * e);
int ti_user_set_pass(ti_user_t * user, const char * pass);
int ti_user_info_to_pk(ti_user_t * user, msgpack_packer * pk);
ti_val_t * ti_user_as_mpval(ti_user_t * user);
ti_token_t * ti_user_pop_token_by_key(ti_user_t * user, ti_token_key_t * key);

struct ti_user_s
{
    uint32_t ref;
    uint64_t id;
    uint64_t created_at;    /* UNIX time-stamp in seconds */
    ti_raw_t * name;
    char * encpass;         /* may be NULL if no password is set */
    vec_t * tokens;         /* ti_token_t */
};

static inline int ti_user_add_token(ti_user_t * user, ti_token_t * token)
{
    return vec_push(&user->tokens, token);
}

#endif /* TI_USER_H_ */
