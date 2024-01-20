/*
 * token.h - cleri token element. note that one single char will parse
 *           slightly faster compared to tokens containing more characters.
 *           (be careful a token should not match the keyword regular
 *           expression)
 */
#ifndef CLERI_TOKEN_H_
#define CLERI_TOKEN_H_

#include <stddef.h>
#include <inttypes.h>
#include <cleri/cleri.h>

/* typedefs */
typedef struct cleri_s cleri_t;
typedef struct cleri_token_s cleri_token_t;

/* public functions */
#ifdef __cplusplus
extern "C" {
#endif

cleri_t * cleri_token(uint32_t gid, const char * token);

#ifdef __cplusplus
}
#endif

/* structs */
struct cleri_token_s
{
    const char * token;
    size_t len;
};

#endif /* CLERI_TOKEN_H_ */
