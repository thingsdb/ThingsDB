/*
 * grammar.h - this should contain the 'start' or your grammar.
 */
#ifndef CLERI_GRAMMAR_H_
#define CLERI_GRAMMAR_H_

#define PCRE2_CODE_UNIT_WIDTH 8

#include <pcre2.h>
#include <cleri/cleri.h>
#include <cleri/olist.h>

#define CLERI_DEFAULT_RE_KEYWORDS "^\\w+"
#define CLERI_DEFAULT_RE_WHITESPACE "\\s*"

/* typedefs */
typedef struct cleri_s cleri_t;
typedef struct cleri_grammar_s cleri_grammar_t;

/* public functions */
#ifdef __cplusplus
extern "C" {
#endif

cleri_grammar_t * cleri_grammar2(
        cleri_t * start,
        const char * re_keywords,
        const char * re_whitespace);
cleri_grammar_t * cleri_grammar(cleri_t * start, const char * re_keywords);
void cleri_grammar_free(cleri_grammar_t * grammar);

#ifdef __cplusplus
}
#endif

/* structs */
struct cleri_grammar_s
{
    cleri_t * start;
    pcre2_code * re_keywords;
    pcre2_match_data * md_keywords;
    pcre2_code * re_whitespace;
    pcre2_match_data * md_whitespace;
};

#endif /* CLERI_GRAMMAR_H_ */
