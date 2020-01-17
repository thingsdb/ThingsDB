/*
 * ti/token.c
 */
#include <ti/token.h>
#include <stdlib.h>
#include <string.h>
#include <util/util.h>

ti_token_t * ti_token_create(
        ti_token_key_t * key,
        uint64_t expire_ts,
        uint64_t created_at,
        const char * description,
        size_t description_sz)
{
    ti_token_t * token = malloc(sizeof(ti_token_t));
    if (!token)
        return NULL;

    token->expire_ts = expire_ts;
    token->created_at = created_at;
    token->description = strndup(description, description_sz);
    if (!token->description)
    {
        free(token);
        return NULL;
    }

    if (key)
        memcpy(token->key, key, sizeof(ti_token_key_t));
    else
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
