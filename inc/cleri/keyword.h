/*
 * keyword.h - cleri keyword element.
 */
#ifndef CLERI_KEYWORD_H_
#define CLERI_KEYWORD_H_

#include <stddef.h>
#include <inttypes.h>
#include <cleri/cleri.h>

/* typedefs */
typedef struct cleri_s cleri_t;
typedef struct cleri_keyword_s cleri_keyword_t;

/* public functions */
#ifdef __cplusplus
extern "C" {
#endif

cleri_t * cleri_keyword(uint32_t gid, const char * keyword, int ign_case);

#ifdef __cplusplus
}
#endif
/* structs */
struct cleri_keyword_s
{
    const char * keyword;
    int ign_case;
    size_t len;
};

#endif /* CLERI_KEYWORD_H_ */