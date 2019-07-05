/*
 * ti/token.h
 */
#ifndef TI_TOKEN_H_
#define TI_TOKEN_H_

typedef struct ti_token_s ti_token_t;
typedef char ti_token_key_t[40];            /* token using 40 characters */

#include <stdint.h>
#include <ti/raw.h>
#include <ti/val.h>
#include <ti/ex.h>

ti_token_t * ti_token_create(
        ti_token_key_t * key,       /* NULL will generate a new key */
        uint64_t expire_ts,
        const char * description,
        size_t description_sz);
void ti_token_destroy(ti_token_t * token);

struct ti_token_s
{
    ti_token_key_t key;         /* contains the token key */
    uint64_t expire_ts;         /* 0 if not set */
    char * description;         /* may be empty */
};


#endif /* TI_TOKEN_H_ */
