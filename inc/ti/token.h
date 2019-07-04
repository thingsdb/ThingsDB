/*
 * ti/token.h
 */
#ifndef TI_TOKEN_H_
#define TI_TOKEN_H_

#define TI_TOKEN_SZ 40

typedef struct ti_token_s  ti_token_t;
typedef char ti_token_key_t[TI_TOKEN_SZ];

#include <stdint.h>
#include <ti/raw.h>
#include <ti/val.h>
#include <ti/ex.h>


ti_token_t * ti_token_create(uint64_t expire_ts, const char * name);
void ti_token_destroy(ti_token_t * token);

struct ti_token_s
{
    char key[TI_TOKEN_SZ];      /* contains the token key */
    uint64_t expire_ts;         /* 0 if not set */
    char * name;                /* may be empty */
};

#endif /* TI_TOKEN_H_ */
