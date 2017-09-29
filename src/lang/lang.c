/*
 * lang/lang.c
 *
 * This grammar is generated using the Grammar.export_c() method and
 * should be used with the libcleri module.
 *
 * Source class: RqlLang
 * Created at: 2017-09-29 22:49:23
 */

#include "lang/lang.h"
#include <stdio.h>

#define CLERI_CASE_SENSITIVE 0
#define CLERI_CASE_INSENSITIVE 1

#define CLERI_FIRST_MATCH 0
#define CLERI_MOST_GREEDY 1

cleri_grammar_t * compile_grammar(void)
{
    cleri_t * r_uint = cleri_regex(CLERI_GID_R_UINT, "^[0-9]+");
    cleri_t * r_int = cleri_regex(CLERI_GID_R_INT, "^[-+]?[0-9]+");
    cleri_t * r_float = cleri_regex(CLERI_GID_R_FLOAT, "^[-+]?[0-9]*\\.?[0-9]+");
    cleri_t * r_str = cleri_regex(CLERI_GID_R_STR, "^(?:\"(?:[^\"]*)\")+");
    cleri_t * r_name = cleri_regex(CLERI_GID_R_NAME, "^[A-Za-z][A-Za-z0-9_]*");
    cleri_t * elem_id = cleri_dup(CLERI_GID_ELEM_ID, r_uint);
    cleri_t * kind = cleri_dup(CLERI_GID_KIND, r_name);
    cleri_t * prop = cleri_dup(CLERI_GID_PROP, r_name);
    cleri_t * mark = cleri_dup(CLERI_GID_MARK, r_name);
    cleri_t * compare_opr = cleri_tokens(CLERI_GID_COMPARE_OPR, "== != <= >= !~ < > ~");
    cleri_t * t_and = cleri_token(CLERI_GID_T_AND, "&&");
    cleri_t * t_or = cleri_token(CLERI_GID_T_OR, "||");
    cleri_t * filter = cleri_sequence(
        CLERI_GID_FILTER,
        3,
        cleri_token(CLERI_NONE, "{"),
        cleri_optional(CLERI_NONE, cleri_prio(
            CLERI_NONE,
            4,
            cleri_sequence(
                CLERI_NONE,
                3,
                prop,
                compare_opr,
                cleri_choice(
                    CLERI_NONE,
                    CLERI_FIRST_MATCH,
                    4,
                    r_uint,
                    r_int,
                    r_float,
                    r_str
                )
            ),
            cleri_sequence(
                CLERI_NONE,
                3,
                cleri_token(CLERI_NONE, "("),
                CLERI_THIS,
                cleri_token(CLERI_NONE, ")")
            ),
            cleri_sequence(
                CLERI_NONE,
                3,
                CLERI_THIS,
                t_and,
                CLERI_THIS
            ),
            cleri_sequence(
                CLERI_NONE,
                3,
                CLERI_THIS,
                t_or,
                CLERI_THIS
            )
        )),
        cleri_token(CLERI_NONE, "}")
    );
    cleri_t * selector = cleri_sequence(
        CLERI_GID_SELECTOR,
        3,
        cleri_optional(CLERI_NONE, cleri_token(CLERI_NONE, "!")),
        cleri_choice(
            CLERI_NONE,
            CLERI_FIRST_MATCH,
            2,
            elem_id,
            kind
        ),
        cleri_optional(CLERI_NONE, cleri_choice(
            CLERI_NONE,
            CLERI_FIRST_MATCH,
            2,
            cleri_token(CLERI_NONE, "*"),
            cleri_sequence(
                CLERI_NONE,
                3,
                cleri_token(CLERI_NONE, "("),
                cleri_list(CLERI_NONE, cleri_sequence(
                    CLERI_NONE,
                    3,
                    prop,
                    cleri_optional(CLERI_NONE, cleri_choice(
                        CLERI_NONE,
                        CLERI_FIRST_MATCH,
                        2,
                        cleri_sequence(
                            CLERI_NONE,
                            2,
                            cleri_token(CLERI_NONE, "#"),
                            prop
                        ),
                        cleri_sequence(
                            CLERI_NONE,
                            2,
                            cleri_token(CLERI_NONE, "."),
                            mark
                        )
                    )),
                    cleri_optional(CLERI_NONE, filter)
                ), cleri_token(CLERI_NONE, ","), 1, 0, 0),
                cleri_token(CLERI_NONE, ")")
            )
        ))
    );
    cleri_t * START = cleri_list(CLERI_GID_START, selector, cleri_token(CLERI_NONE, ","), 1, 0, 0);

    cleri_grammar_t * grammar = cleri_grammar(START, "^\\w+");

    return grammar;
}
