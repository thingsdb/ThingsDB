/*
 * cleri.h - each cleri element is a cleri object.
 */
#ifndef CLERI_OBJECT_H_
#define CLERI_OBJECT_H_

#ifdef __cplusplus
#define cleri__malloc(__t) ((__t*)malloc(sizeof(__t)))
#else
#define cleri__malloc(__t) (malloc(sizeof(__t)))
#endif

#ifdef __cplusplus
#define cleri__mallocn(__n, __t) ((__t*)malloc(__n * sizeof(__t)))
#else
#define cleri__mallocn(__n, __t) (malloc(__n * sizeof(__t)))
#endif

#include <cleri/expecting.h>
#include <cleri/keyword.h>
#include <cleri/sequence.h>
#include <cleri/optional.h>
#include <cleri/choice.h>
#include <cleri/dup.h>
#include <cleri/list.h>
#include <cleri/regex.h>
#include <cleri/repeat.h>
#include <cleri/token.h>
#include <cleri/tokens.h>
#include <cleri/grammar.h>
#include <cleri/prio.h>
#include <cleri/node.h>
#include <cleri/parse.h>
#include <cleri/rule.h>
#include <cleri/this.h>
#include <cleri/ref.h>
#include <cleri/version.h>

/* typedefs */
typedef struct cleri_s cleri_t;
typedef struct cleri_grammar_s cleri_grammar_t;
typedef struct cleri_keyword_s cleri_keyword_t;
typedef struct cleri_sequence_s cleri_sequence_t;
typedef struct cleri_optional_s cleri_optional_t;
typedef struct cleri_choice_s cleri_choice_t;
typedef struct cleri_regex_s cleri_regex_t;
typedef struct cleri_list_s cleri_list_t;
typedef struct cleri_repeat_s cleri_repeat_t;
typedef struct cleri_token_s cleri_token_t;
typedef struct cleri_tokens_s cleri_tokens_t;
typedef struct cleri_prio_s cleri_prio_t;
typedef struct cleri_rule_s cleri_rule_t;
typedef struct cleri_rule_store_s cleri_rule_store_t;
typedef struct cleri_node_s cleri_node_t;
typedef struct cleri_parse_s cleri_parse_t;
typedef struct cleri_ref_s cleri_ref_t;
typedef struct cleri_s cleri_t;
typedef struct cleri_dup_s cleri_dup_t;

typedef union cleri_u cleri_via_t;

typedef void (*cleri_free_object_t)(cleri_t *);
typedef cleri_node_t * (*cleri_parse_object_t)(
        cleri_parse_t *,
        cleri_node_t *,
        cleri_t *,
        cleri_rule_store_t *);

/* enums */
typedef enum cleri_e {
    CLERI_TP_SEQUENCE,
    CLERI_TP_OPTIONAL,
    CLERI_TP_CHOICE,
    CLERI_TP_LIST,
    CLERI_TP_REPEAT,
    CLERI_TP_PRIO,
    CLERI_TP_RULE,
    CLERI_TP_THIS,
    /* all items after this will not get children */
    CLERI_TP_KEYWORD,
    CLERI_TP_TOKEN,
    CLERI_TP_TOKENS,
    CLERI_TP_REGEX,
    CLERI_TP_REF,
    CLERI_TP_END_OF_STATEMENT
} cleri_tp;

/* unions */
union cleri_u
{
    cleri_keyword_t * keyword;
    cleri_sequence_t * sequence;
    cleri_optional_t * optional;
    cleri_choice_t * choice;
    cleri_regex_t * regex;
    cleri_list_t * list;
    cleri_repeat_t * repeat;
    cleri_token_t * token;
    cleri_tokens_t * tokens;
    cleri_prio_t * prio;
    cleri_rule_t * rule;
    cleri_dup_t * dup;
    void * dummy; /* place holder */
};

/* public functions */
#ifdef __cplusplus
extern "C" {
#endif

cleri_t * cleri_new(
        uint32_t gid,
        cleri_tp tp,
        cleri_free_object_t free_object,
        cleri_parse_object_t parse_object);
void cleri_incref(cleri_t * cl_object);
void cleri_decref(cleri_t * cl_object);
int cleri_free(cleri_t * cl_object);

#ifdef __cplusplus
}
#endif

/* fixed end of statement object */
extern cleri_t * CLERI_END_OF_STATEMENT;

/* structs */

#define CLERI_OBJECT_FIELDS                 \
    uint32_t gid;                           \
    uint32_t ref;                           \
    cleri_free_object_t free_object;        \
    cleri_parse_object_t parse_object;      \
    cleri_tp tp;                            \
    cleri_via_t via;

struct cleri_s
{
    CLERI_OBJECT_FIELDS
};

struct cleri_dup_s
{
    CLERI_OBJECT_FIELDS
    cleri_t * dup;
};

#endif /* CLERI_OBJECT_H_ */