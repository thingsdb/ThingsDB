/*
 * ti/user.t.h
 */
#ifndef TI_USER_T_H_
#define TI_USER_T_H_

typedef struct ti_user_s  ti_user_t;

extern const char * ti_user_def_name;
extern const char * ti_user_def_pass;
extern const unsigned int ti_min_name;
extern const unsigned int ti_max_name;
extern const unsigned int ti_min_pass;
extern const unsigned int ti_max_pass;

#include <inttypes.h>
#include <ti/raw.t.h>
#include <util/vec.h>

struct ti_user_s
{
    uint32_t ref;
    uint64_t id;
    uint64_t created_at;    /* UNIX time-stamp in seconds */
    ti_raw_t * name;
    char * encpass;         /* may be NULL if no password is set */
    vec_t * tokens;         /* ti_token_t */
    vec_t * whitelists[2];  /* For the vectors:
                             *   TI_WHITELIST_ROOMS
                             *   TI_WHITELIST_PROCEDURES
                             * Both may be NULL, contains ti_val_t */
};

#endif  /* TI_USER_T_H_ */
