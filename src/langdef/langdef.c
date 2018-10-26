/*
 * langdef.c
 *
 * This grammar is generated using the Grammar.export_c() method and
 * should be used with the libcleri module.
 *
 * Source class: Definition
 * Created at: 2018-10-26 12:05:19
 */

#include <langdef/langdef.h>
#include <stdio.h>

#define CLERI_CASE_SENSITIVE 0
#define CLERI_CASE_INSENSITIVE 1

#define CLERI_FIRST_MATCH 0
#define CLERI_MOST_GREEDY 1

cleri_grammar_t * compile_langdef(void)
{
    cleri_t * r_single_quote = cleri_regex(CLERI_GID_R_SINGLE_QUOTE, "^(?:\'(?:[^\']*)\')+");
    cleri_t * r_double_quote = cleri_regex(CLERI_GID_R_DOUBLE_QUOTE, "^(?:\"(?:[^\"]*)\")+");
    cleri_t * t_string = cleri_choice(
        CLERI_GID_T_STRING,
        CLERI_FIRST_MATCH,
        2,
        r_single_quote,
        r_double_quote
    );
    cleri_t * t_nil = cleri_keyword(CLERI_GID_T_NIL, "nil", CLERI_CASE_SENSITIVE);
    cleri_t * t_false = cleri_keyword(CLERI_GID_T_FALSE, "false", CLERI_CASE_SENSITIVE);
    cleri_t * t_true = cleri_keyword(CLERI_GID_T_TRUE, "true", CLERI_CASE_SENSITIVE);
    cleri_t * t_int = cleri_regex(CLERI_GID_T_INT, "^[-+]?[0-9]+");
    cleri_t * t_float = cleri_regex(CLERI_GID_T_FLOAT, "^[-+]?[0-9]*\\.?[0-9]+");
    cleri_t * comment = cleri_optional(CLERI_GID_COMMENT, cleri_repeat(CLERI_NONE, cleri_regex(CLERI_NONE, "^(?s)/\\\\*.*?\\\\*/"), 0, 0));
    cleri_t * identifier = cleri_regex(CLERI_GID_IDENTIFIER, "^[a-zA-Z_][a-zA-Z0-9_]*");
    cleri_t * index = cleri_sequence(
        CLERI_GID_INDEX,
        3,
        cleri_token(CLERI_NONE, "["),
        cleri_choice(
            CLERI_NONE,
            CLERI_FIRST_MATCH,
            2,
            identifier,
            t_int
        ),
        cleri_token(CLERI_NONE, "]")
    );
    cleri_t * f_blob = cleri_keyword(CLERI_GID_F_BLOB, "blob", CLERI_CASE_SENSITIVE);
    cleri_t * f_fetch = cleri_keyword(CLERI_GID_F_FETCH, "fetch", CLERI_CASE_SENSITIVE);
    cleri_t * f_map = cleri_keyword(CLERI_GID_F_MAP, "map", CLERI_CASE_SENSITIVE);
    cleri_t * f_thing = cleri_keyword(CLERI_GID_F_THING, "thing", CLERI_CASE_SENSITIVE);
    cleri_t * f_create = cleri_keyword(CLERI_GID_F_CREATE, "create", CLERI_CASE_SENSITIVE);
    cleri_t * f_delete = cleri_keyword(CLERI_GID_F_DELETE, "delete", CLERI_CASE_SENSITIVE);
    cleri_t * f_drop = cleri_keyword(CLERI_GID_F_DROP, "drop", CLERI_CASE_SENSITIVE);
    cleri_t * f_grant = cleri_keyword(CLERI_GID_F_GRANT, "grant", CLERI_CASE_SENSITIVE);
    cleri_t * f_push = cleri_keyword(CLERI_GID_F_PUSH, "push", CLERI_CASE_SENSITIVE);
    cleri_t * f_rename = cleri_keyword(CLERI_GID_F_RENAME, "rename", CLERI_CASE_SENSITIVE);
    cleri_t * f_revoke = cleri_keyword(CLERI_GID_F_REVOKE, "revoke", CLERI_CASE_SENSITIVE);
    cleri_t * primitives = cleri_choice(
        CLERI_GID_PRIMITIVES,
        CLERI_FIRST_MATCH,
        6,
        t_nil,
        t_false,
        t_true,
        t_int,
        t_float,
        t_string
    );
    cleri_t * scope = cleri_ref();
    cleri_t * value = cleri_ref();
    cleri_t * thing = cleri_sequence(
        CLERI_GID_THING,
        3,
        cleri_token(CLERI_NONE, "{"),
        cleri_list(CLERI_NONE, cleri_sequence(
            CLERI_NONE,
            3,
            identifier,
            cleri_token(CLERI_NONE, ":"),
            value
        ), cleri_token(CLERI_NONE, ","), 0, 0, 1),
        cleri_token(CLERI_NONE, "}")
    );
    cleri_t * array = cleri_sequence(
        CLERI_GID_ARRAY,
        3,
        cleri_token(CLERI_NONE, "["),
        cleri_list(CLERI_NONE, value, cleri_token(CLERI_NONE, ","), 0, 0, 1),
        cleri_token(CLERI_NONE, "]")
    );
    cleri_t * iterator = cleri_sequence(
        CLERI_GID_ITERATOR,
        3,
        cleri_list(CLERI_NONE, identifier, cleri_token(CLERI_NONE, ","), 1, 2, 0),
        cleri_token(CLERI_NONE, "=>"),
        scope
    );
    cleri_t * assignment = cleri_sequence(
        CLERI_GID_ASSIGNMENT,
        2,
        cleri_token(CLERI_NONE, "="),
        value
    );
    cleri_t * function = cleri_sequence(
        CLERI_GID_FUNCTION,
        4,
        cleri_choice(
            CLERI_NONE,
            CLERI_FIRST_MATCH,
            12,
            f_blob,
            f_fetch,
            f_map,
            f_thing,
            f_create,
            f_delete,
            f_drop,
            f_grant,
            f_push,
            f_rename,
            f_revoke,
            identifier
        ),
        cleri_token(CLERI_NONE, "("),
        cleri_choice(
            CLERI_NONE,
            CLERI_FIRST_MATCH,
            2,
            iterator,
            cleri_list(CLERI_NONE, value, cleri_token(CLERI_NONE, ","), 0, 0, 1)
        ),
        cleri_token(CLERI_NONE, ")")
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
            function,
            identifier
        ),
        cleri_optional(CLERI_NONE, index),
        cleri_optional(CLERI_NONE, cleri_choice(
            CLERI_NONE,
            CLERI_FIRST_MATCH,
            2,
            cleri_sequence(
                CLERI_NONE,
                2,
                cleri_token(CLERI_NONE, "."),
                scope
            ),
            assignment
        ))
    ));
    cleri_ref_set(value, cleri_choice(
        CLERI_GID_VALUE,
        CLERI_FIRST_MATCH,
        4,
        primitives,
        thing,
        array,
        scope
    ));

    cleri_grammar_t * grammar = cleri_grammar(START, "^[a-zA-Z_][a-zA-Z0-9_]*");

    return grammar;
}
