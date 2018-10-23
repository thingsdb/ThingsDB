/*
 * langdef.c
 *
 * This grammar is generated using the Grammar.export_c() method and
 * should be used with the libcleri module.
 *
 * Source class: Definition
 * Created at: 2018-10-22 13:59:55
 */

#include <langdef/langdef.h>
#include <stdio.h>

#define CLERI_CASE_SENSITIVE 0
#define CLERI_CASE_INSENSITIVE 1

#define CLERI_FIRST_MATCH 0
#define CLERI_MOST_GREEDY 1

cleri_grammar_t * compile_langdef(void)
{
    cleri_t * r_single_quote_str = cleri_regex(CLERI_GID_R_SINGLE_QUOTE_STR, "^(?:\'(?:[^\']*)\')+");
    cleri_t * r_double_quote_str = cleri_regex(CLERI_GID_R_DOUBLE_QUOTE_STR, "^(?:\"(?:[^\"]*)\")+");
    cleri_t * r_int = cleri_regex(CLERI_GID_R_INT, "^[-+]?[0-9]+");
    cleri_t * r_float = cleri_regex(CLERI_GID_R_FLOAT, "^[-+]?[0-9]*\\.?[0-9]+");
    cleri_t * r_comment = cleri_regex(CLERI_GID_R_COMMENT, "^(?s)/\\\\*.*?\\\\*/");
    cleri_t * array_idx = cleri_regex(CLERI_GID_ARRAY_IDX, "^[-+]?[0-9]+");
    cleri_t * thing_id = cleri_regex(CLERI_GID_THING_ID, "^[0-9]+");
    cleri_t * comment = cleri_optional(CLERI_GID_COMMENT, cleri_repeat(CLERI_NONE, r_comment, 0, 0));
    cleri_t * identifier = cleri_regex(CLERI_GID_IDENTIFIER, "^[a-zA-Z_][a-zA-Z0-9_]*");
    cleri_t * string = cleri_choice(
        CLERI_GID_STRING,
        CLERI_FIRST_MATCH,
        2,
        r_single_quote_str,
        r_double_quote_str
    );
    cleri_t * scope = cleri_ref();
    cleri_t * chain = cleri_ref();
    cleri_t * primitives = cleri_choice(
        CLERI_GID_PRIMITIVES,
        CLERI_FIRST_MATCH,
        6,
        cleri_keyword(CLERI_NONE, "nil", CLERI_CASE_SENSITIVE),
        cleri_keyword(CLERI_NONE, "false", CLERI_CASE_SENSITIVE),
        cleri_keyword(CLERI_NONE, "true", CLERI_CASE_SENSITIVE),
        r_int,
        r_float,
        string
    );
    cleri_t * auth_flags = cleri_list(CLERI_GID_AUTH_FLAGS, cleri_choice(
        CLERI_NONE,
        CLERI_FIRST_MATCH,
        5,
        cleri_keyword(CLERI_NONE, "FULL", CLERI_CASE_SENSITIVE),
        cleri_keyword(CLERI_NONE, "ACCCESS", CLERI_CASE_SENSITIVE),
        cleri_keyword(CLERI_NONE, "READ", CLERI_CASE_SENSITIVE),
        cleri_keyword(CLERI_NONE, "MODIFY", CLERI_CASE_SENSITIVE),
        cleri_regex(CLERI_NONE, "^[0-9]+")
    ), cleri_token(CLERI_NONE, "|"), 0, 0, 0);
    cleri_t * f_blob = cleri_sequence(
        CLERI_GID_F_BLOB,
        4,
        cleri_keyword(CLERI_NONE, "blob", CLERI_CASE_SENSITIVE),
        cleri_token(CLERI_NONE, "("),
        cleri_optional(CLERI_NONE, array_idx),
        cleri_token(CLERI_NONE, ")")
    );
    cleri_t * f_thing = cleri_sequence(
        CLERI_GID_F_THING,
        4,
        cleri_keyword(CLERI_NONE, "thing", CLERI_CASE_SENSITIVE),
        cleri_token(CLERI_NONE, "("),
        cleri_optional(CLERI_NONE, thing_id),
        cleri_token(CLERI_NONE, ")")
    );
    cleri_t * f_create = cleri_sequence(
        CLERI_GID_F_CREATE,
        4,
        cleri_keyword(CLERI_NONE, "create", CLERI_CASE_SENSITIVE),
        cleri_token(CLERI_NONE, "("),
        cleri_list(CLERI_NONE, identifier, cleri_token(CLERI_NONE, ","), 0, 0, 1),
        cleri_token(CLERI_NONE, ")")
    );
    cleri_t * f_grant = cleri_sequence(
        CLERI_GID_F_GRANT,
        4,
        cleri_keyword(CLERI_NONE, "grant", CLERI_CASE_SENSITIVE),
        cleri_token(CLERI_NONE, "("),
        auth_flags,
        cleri_token(CLERI_NONE, ")")
    );
    cleri_t * f_revoke = cleri_sequence(
        CLERI_GID_F_REVOKE,
        4,
        cleri_keyword(CLERI_NONE, "revoke", CLERI_CASE_SENSITIVE),
        cleri_token(CLERI_NONE, "("),
        auth_flags,
        cleri_token(CLERI_NONE, ")")
    );
    cleri_t * f_drop = cleri_sequence(
        CLERI_GID_F_DROP,
        3,
        cleri_keyword(CLERI_NONE, "drop", CLERI_CASE_SENSITIVE),
        cleri_token(CLERI_NONE, "("),
        cleri_token(CLERI_NONE, ")")
    );
    cleri_t * f_delete = cleri_sequence(
        CLERI_GID_F_DELETE,
        3,
        cleri_keyword(CLERI_NONE, "delete", CLERI_CASE_SENSITIVE),
        cleri_token(CLERI_NONE, "("),
        cleri_token(CLERI_NONE, ")")
    );
    cleri_t * f_rename = cleri_sequence(
        CLERI_GID_F_RENAME,
        4,
        cleri_keyword(CLERI_NONE, "rename", CLERI_CASE_SENSITIVE),
        cleri_token(CLERI_NONE, "("),
        identifier,
        cleri_token(CLERI_NONE, ")")
    );
    cleri_t * f_fetch = cleri_sequence(
        CLERI_GID_F_FETCH,
        4,
        cleri_keyword(CLERI_NONE, "fetch", CLERI_CASE_SENSITIVE),
        cleri_token(CLERI_NONE, "("),
        cleri_list(CLERI_NONE, identifier, cleri_token(CLERI_NONE, ","), 0, 0, 1),
        cleri_token(CLERI_NONE, ")")
    );
    cleri_t * f_map = cleri_sequence(
        CLERI_GID_F_MAP,
        6,
        cleri_keyword(CLERI_NONE, "map", CLERI_CASE_SENSITIVE),
        cleri_token(CLERI_NONE, "("),
        cleri_list(CLERI_NONE, identifier, cleri_token(CLERI_NONE, ","), 1, 2, 0),
        cleri_token(CLERI_NONE, "=>"),
        scope,
        cleri_token(CLERI_NONE, ")")
    );
    cleri_t * action = cleri_choice(
        CLERI_GID_ACTION,
        CLERI_FIRST_MATCH,
        2,
        cleri_sequence(
            CLERI_NONE,
            2,
            cleri_token(CLERI_NONE, "."),
            chain
        ),
        cleri_sequence(
            CLERI_NONE,
            2,
            cleri_token(CLERI_NONE, "="),
            cleri_choice(
                CLERI_NONE,
                CLERI_FIRST_MATCH,
                3,
                primitives,
                f_blob,
                scope
            )
        )
    );
    cleri_t * statement = cleri_sequence(
        CLERI_GID_STATEMENT,
        2,
        comment,
        scope
    );
    cleri_t * START = cleri_sequence(
        CLERI_GID_START,
        2,
        cleri_list(CLERI_NONE, statement, cleri_token(CLERI_NONE, ";"), 0, 0, 1),
        comment
    );
    cleri_ref_set(scope, cleri_sequence(
        CLERI_GID_SCOPE,
        3,
        cleri_choice(
            CLERI_NONE,
            CLERI_FIRST_MATCH,
            2,
            f_thing,
            identifier
        ),
        cleri_optional(CLERI_NONE, cleri_sequence(
            CLERI_NONE,
            3,
            cleri_token(CLERI_NONE, "["),
            cleri_choice(
                CLERI_NONE,
                CLERI_FIRST_MATCH,
                2,
                identifier,
                array_idx
            ),
            cleri_token(CLERI_NONE, "]")
        )),
        cleri_optional(CLERI_NONE, action)
    ));
    cleri_ref_set(chain, cleri_sequence(
        CLERI_GID_CHAIN,
        3,
        cleri_choice(
            CLERI_NONE,
            CLERI_FIRST_MATCH,
            9,
            f_create,
            f_drop,
            f_rename,
            f_fetch,
            f_grant,
            f_revoke,
            f_delete,
            f_map,
            identifier
        ),
        cleri_optional(CLERI_NONE, cleri_sequence(
            CLERI_NONE,
            3,
            cleri_token(CLERI_NONE, "["),
            cleri_choice(
                CLERI_NONE,
                CLERI_FIRST_MATCH,
                2,
                identifier,
                array_idx
            ),
            cleri_token(CLERI_NONE, "]")
        )),
        cleri_optional(CLERI_NONE, action)
    ));

    cleri_grammar_t * grammar = cleri_grammar(START, "^[a-zA-Z_][a-zA-Z0-9_]*");

    return grammar;
}
