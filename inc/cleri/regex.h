/*
 * regex.h - cleri regular expression element.
 */
#ifndef CLERI_REGEX_H_
#define CLERI_REGEX_H_

#define PCRE2_CODE_UNIT_WIDTH 8

#include <pcre2.h>
#include <stddef.h>
#include <inttypes.h>
#include <cleri/cleri.h>

/* typedefs */
typedef struct cleri_s cleri_t;
typedef struct cleri_regex_s cleri_regex_t;

/* public functions */
#ifdef __cplusplus
extern "C" {
#endif

cleri_t * cleri_regex(uint32_t gid, const char * pattern);

#ifdef __cplusplus
}
#endif

/* structs */
struct cleri_regex_s
{
    pcre2_code * regex;
    pcre2_match_data * match_data;
};

#endif /* CLERI_REGEX_H_ */