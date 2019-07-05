/*
 * ti/token.c
 */
#include <ti/token.h>
#include <stdlib.h>
#include <string.h>
#include <util/util.h>

ti_token_t * ti_token_create(
        uint64_t expire_ts,
        const char * description,
        size_t description_sz)
{
    ti_token_t * token = malloc(sizeof(ti_token_t));
    if (!token)
        return NULL;

    token->expire_ts = expire_ts;
    token->description = strndup(description, description_sz);
    if (!token->description)
    {
        free(token);
        return NULL;
    }

    util_random_key(token->key, sizeof(ti_token_key_t));

    return token;
}

void ti_token_destroy(ti_token_t * token)
{
    if (!token)
        return;
    free(token->description);
    free(token);
}
