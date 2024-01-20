/*
 * tokens.h - cleri tokens element. (like token but can contain more tokens
 *            in one element)
 */
#ifndef CLERI_TOKENS_H_
#define CLERI_TOKENS_H_

#include <stddef.h>
#include <inttypes.h>
#include <cleri/cleri.h>

/* typedefs */
typedef struct cleri_s cleri_t;
typedef struct cleri_tlist_s cleri_tlist_t;
typedef struct cleri_tokens_s cleri_tokens_t;

/* public functions */
#ifdef __cplusplus
extern "C" {
#endif

cleri_t * cleri_tokens(uint32_t gid, const char * tokens);

#ifdef __cplusplus
}
#endif

/* structs */
struct cleri_tlist_s
{
    const char * token;
    size_t len;
    cleri_tlist_t * next;
};

struct cleri_tokens_s
{
    char * tokens;
    char * spaced;
    cleri_tlist_t * tlist;
};

#endif /* CLERI_TOKENS_H_ */