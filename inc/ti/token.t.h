/*
 * ti/token.t.h
 */
#ifndef TI_TOKEN_T_H_
#define TI_TOKEN_T_H_

typedef struct ti_token_s ti_token_t;
typedef char ti_token_key_t[22];    /* token using 22 base64 characters */

#include <stdint.h>

struct ti_token_s
{
    ti_token_key_t key;         /* contains the token key */
    uint64_t expire_ts;         /* 0 if not set (UNIX time in seconds) */
    uint64_t created_at;        /* UNIX time in seconds */
    char * description;         /* may be empty */
};

#endif /* TI_TOKEN_T_H_ */
