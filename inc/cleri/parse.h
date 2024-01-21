/*
 * parse.h - this contains everything for parsing a string to a grammar.
 */
#ifndef CLERI_PARSE_H_
#define CLERI_PARSE_H_

#include <stddef.h>
#include <stdbool.h>
#include <cleri/cleri.h>
#include <cleri/grammar.h>
#include <cleri/node.h>
#include <cleri/expecting.h>
#include <cleri/kwcache.h>
#include <cleri/rule.h>

#ifndef MAX_RECURSION_DEPTH
#define MAX_RECURSION_DEPTH 500
#endif

enum
{
    CLERI_FLAG_EXPECTING_DISABLED   =1<<0,
    CLERI_FLAG_EXCLUDE_OPTIONAL     =1<<1,
    CLERI_FLAG_EXCLUDE_FM_CHOICE    =1<<2,
    CLERI_FLAG_EXCLUDE_RULE_THIS    =1<<3,
};

/* typedefs */
typedef struct cleri_s cleri_t;
typedef struct cleri_grammar_s cleri_grammar_t;
typedef struct cleri_node_s cleri_node_t;
typedef struct cleri_expecting_s cleri_expecting_t;
typedef struct cleri_rule_store_s cleri_rule_store_t;
typedef struct cleri_parse_s cleri_parse_t;
typedef const char * (cleri_translate_t)(cleri_t *);

/* public functions */
#ifdef __cplusplus
extern "C" {
#endif

static inline cleri_parse_t * cleri_parse(
    cleri_grammar_t * grammar,
    const char * str);
cleri_parse_t * cleri_parse2(
    cleri_grammar_t * grammar,
    const char * str,
    int flags);
void cleri_parse_free(cleri_parse_t * pr);
void cleri_parse_expect_start(cleri_parse_t * pr);
int cleri_parse_strn(
    char * s,
    size_t n,
    cleri_parse_t * pr,
    cleri_translate_t * translate);

#ifdef __cplusplus
}
#endif

/* private functions */
cleri_node_t * cleri__parse_walk(
        cleri_parse_t * pr,
        cleri_node_t * parent,
        cleri_t * cl_obj,
        cleri_rule_store_t * rule,
        int mode);

/* structs */
struct cleri_parse_s
{
    int is_valid;
    int flags;
    size_t pos;
    const char * str;
    cleri_node_t * tree;
    const cleri_olist_t * expect;
    cleri_expecting_t * expecting;
    cleri_grammar_t * grammar;
    uint8_t * kwcache;
};

static inline cleri_parse_t * cleri_parse(
    cleri_grammar_t * grammar,
    const char * str)
{
    return cleri_parse2(grammar, str, 0);
}

#endif /* CLERI_PARSE_H_ */
