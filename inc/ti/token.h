/*
 * ti/token.h
 */
#ifndef TI_TOKEN_H_
#define TI_TOKEN_H_

#include <stdlib.h>
#include <stdint.h>
#include <ti/token.t.h>

ti_token_t * ti_token_create(
        ti_token_key_t * key,       /* NULL will generate a new key */
        uint64_t expire_ts,
        uint64_t created_at,
        const char * description,
        size_t description_sz);
void ti_token_destroy(ti_token_t * token);


#endif /* TI_TOKEN_H_ */
