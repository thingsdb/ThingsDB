/*
 * langdef.c
 *
 * This grammar is generated using the Grammar.export_c() method and
 * should be used with the libcleri module.
 *
 * Source class: Definition
 * Created at: 2019-07-12 00:59:18
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
    cleri_t * t_false = cleri_keyword(CLERI_GID_T_FALSE, "false", CLERI_CASE_SENSITIVE);
    cleri_t * t_float = cleri_regex(CLERI_GID_T_FLOAT, "^[-+]?((inf|nan)([^0-9A-Za-z_]|$)|[0-9]*\\.[0-9]+(e[+-][0-9]+)?)");
    cleri_t * t_int = cleri_regex(CLERI_GID_T_INT, "^[-+]?((0b[01]+)|(0o[0-8]+)|(0x[0-9a-fA-F]+)|([0-9]+))");
    cleri_t * t_nil = cleri_keyword(CLERI_GID_T_NIL, "nil", CLERI_CASE_SENSITIVE);
    cleri_t * t_regex = cleri_regex(CLERI_GID_T_REGEX, "^(/[^/\\\\]*(?:\\\\.[^/\\\\]*)*/i?)");
    cleri_t * t_string = cleri_choice(
        CLERI_GID_T_STRING,
        CLERI_FIRST_MATCH,
        2,
        r_single_quote,
        r_double_quote
    );
    cleri_t * t_true = cleri_keyword(CLERI_GID_T_TRUE, "true", CLERI_CASE_SENSITIVE);
    cleri_t * o_not = cleri_repeat(CLERI_GID_O_NOT, cleri_token(CLERI_NONE, "!"), 0, 0);
    cleri_t * comment = cleri_repeat(CLERI_GID_COMMENT, cleri_regex(CLERI_NONE, "^(?s)/\\\\*.*?\\\\*/"), 0, 0);
    cleri_t * name = cleri_regex(CLERI_GID_NAME, "^[A-Za-z_][0-9A-Za-z_]*");
    cleri_t * tmp = cleri_regex(CLERI_GID_TMP, "^\\$[A-Za-z_][0-9A-Za-z_]*");
    cleri_t * primitives = cleri_choice(
        CLERI_GID_PRIMITIVES,
        CLERI_FIRST_MATCH,
        7,
        t_false,
        t_nil,
        t_true,
        t_float,
        t_int,
        t_string,
        t_regex
    );
    cleri_t * scope = cleri_ref();
    cleri_t * chain = cleri_ref();
    cleri_t * thing = cleri_sequence(
        CLERI_GID_THING,
        3,
        cleri_token(CLERI_NONE, "{"),
        cleri_list(CLERI_NONE, cleri_sequence(
            CLERI_NONE,
            3,
            name,
            cleri_token(CLERI_NONE, ":"),
            scope
        ), cleri_token(CLERI_NONE, ","), 0, 0, 1),
        cleri_token(CLERI_NONE, "}")
    );
    cleri_t * array = cleri_sequence(
        CLERI_GID_ARRAY,
        3,
        cleri_token(CLERI_NONE, "["),
        cleri_list(CLERI_NONE, scope, cleri_token(CLERI_NONE, ","), 0, 0, 1),
        cleri_token(CLERI_NONE, "]")
    );
    cleri_t * closure = cleri_sequence(
        CLERI_GID_CLOSURE,
        4,
        cleri_token(CLERI_NONE, "|"),
        cleri_list(CLERI_NONE, name, cleri_token(CLERI_NONE, ","), 0, 0, 1),
        cleri_token(CLERI_NONE, "|"),
        scope
    );
    cleri_t * function = cleri_sequence(
        CLERI_GID_FUNCTION,
        4,
        name,
        cleri_token(CLERI_NONE, "("),
        cleri_list(CLERI_NONE, scope, cleri_token(CLERI_NONE, ","), 0, 0, 1),
        cleri_token(CLERI_NONE, ")")
    );
    cleri_t * opr0_mul_div_mod = cleri_tokens(CLERI_GID_OPR0_MUL_DIV_MOD, "// * / %");
    cleri_t * opr1_add_sub = cleri_tokens(CLERI_GID_OPR1_ADD_SUB, "+ -");
    cleri_t * opr2_bitwise_and = cleri_tokens(CLERI_GID_OPR2_BITWISE_AND, "&");
    cleri_t * opr3_bitwise_xor = cleri_tokens(CLERI_GID_OPR3_BITWISE_XOR, "^");
    cleri_t * opr4_bitwise_or = cleri_tokens(CLERI_GID_OPR4_BITWISE_OR, "|");
    cleri_t * opr5_compare = cleri_tokens(CLERI_GID_OPR5_COMPARE, "== != <= >= < >");
    cleri_t * opr6_cmp_and = cleri_token(CLERI_GID_OPR6_CMP_AND, "&&");
    cleri_t * opr7_cmp_or = cleri_token(CLERI_GID_OPR7_CMP_OR, "||");
    cleri_t * operations = cleri_sequence(
        CLERI_GID_OPERATIONS,
        4,
        cleri_token(CLERI_NONE, "("),
        cleri_prio(
            CLERI_NONE,
            2,
            scope,
            cleri_sequence(
                CLERI_NONE,
                3,
                CLERI_THIS,
                cleri_choice(
                    CLERI_NONE,
                    CLERI_MOST_GREEDY,
                    8,
                    opr0_mul_div_mod,
                    opr1_add_sub,
                    opr2_bitwise_and,
                    opr3_bitwise_xor,
                    opr4_bitwise_or,
                    opr5_compare,
                    opr6_cmp_and,
                    opr7_cmp_or
                ),
                CLERI_THIS
            )
        ),
        cleri_token(CLERI_NONE, ")"),
        cleri_optional(CLERI_NONE, cleri_sequence(
            CLERI_NONE,
            4,
            cleri_token(CLERI_NONE, "?"),
            scope,
            cleri_token(CLERI_NONE, ":"),
            scope
        ))
    );
    cleri_t * assignment = cleri_sequence(
        CLERI_GID_ASSIGNMENT,
        3,
        name,
        cleri_tokens(CLERI_NONE, "+= -= *= /= %= &= ^= |= ="),
        scope
    );
    cleri_t * tmp_assign = cleri_sequence(
        CLERI_GID_TMP_ASSIGN,
        3,
        tmp,
        cleri_tokens(CLERI_NONE, "+= -= *= /= %= &= ^= |= ="),
        scope
    );
    cleri_t * index = cleri_repeat(CLERI_GID_INDEX, cleri_sequence(
        CLERI_NONE,
        3,
        cleri_token(CLERI_NONE, "["),
        scope,
        cleri_token(CLERI_NONE, "]")
    ), 0, 0);
    cleri_t * statements = cleri_list(CLERI_GID_STATEMENTS, scope, cleri_sequence(
        CLERI_NONE,
        2,
        cleri_token(CLERI_NONE, ";"),
        comment
    ), 0, 0, 1);
    cleri_t * START = cleri_sequence(
        CLERI_GID_START,
        2,
        comment,
        statements
    );
    cleri_ref_set(scope, cleri_sequence(
        CLERI_GID_SCOPE,
        4,
        o_not,
        cleri_choice(
            CLERI_NONE,
            CLERI_FIRST_MATCH,
            10,
            primitives,
            function,
            assignment,
            tmp_assign,
            name,
            closure,
            tmp,
            thing,
            array,
            operations
        ),
        index,
        cleri_optional(CLERI_NONE, chain)
    ));
    cleri_ref_set(chain, cleri_sequence(
        CLERI_GID_CHAIN,
        4,
        cleri_token(CLERI_NONE, "."),
        cleri_choice(
            CLERI_NONE,
            CLERI_FIRST_MATCH,
            3,
            function,
            assignment,
            name
        ),
        index,
        cleri_optional(CLERI_NONE, chain)
    ));

    cleri_grammar_t * grammar = cleri_grammar(START, "^[A-Za-z_][0-9A-Za-z_]*");

    return grammar;
}
