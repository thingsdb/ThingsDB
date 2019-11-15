/*
 * langdef.c
 *
 * This grammar is generated using the Grammar.export_c() method and
 * should be used with the libcleri module.
 *
 * Source class: LangDef
 * Created at: 2019-11-15 13:47:55
 */

#include <langdef/langdef.h>
#include <stdio.h>

#define CLERI_CASE_SENSITIVE 0
#define CLERI_CASE_INSENSITIVE 1

#define CLERI_FIRST_MATCH 0
#define CLERI_MOST_GREEDY 1

cleri_grammar_t * compile_langdef(void)
{
    cleri_t * x_array = cleri_token(CLERI_GID_X_ARRAY, "[");
    cleri_t * x_assign = cleri_tokens(CLERI_GID_X_ASSIGN, "+= -= *= /= %= &= ^= |= =");
    cleri_t * x_block = cleri_token(CLERI_GID_X_BLOCK, "{");
    cleri_t * x_chain = cleri_token(CLERI_GID_X_CHAIN, ".");
    cleri_t * x_closure = cleri_token(CLERI_GID_X_CLOSURE, "|");
    cleri_t * x_function = cleri_token(CLERI_GID_X_FUNCTION, "(");
    cleri_t * x_index = cleri_token(CLERI_GID_X_INDEX, "[");
    cleri_t * x_not = cleri_token(CLERI_GID_X_NOT, "!");
    cleri_t * x_parenthesis = cleri_token(CLERI_GID_X_PARENTHESIS, "(");
    cleri_t * x_ternary = cleri_token(CLERI_GID_X_TERNARY, "?");
    cleri_t * x_thing = cleri_token(CLERI_GID_X_THING, "{");
    cleri_t * r_single_quote = cleri_regex(CLERI_GID_R_SINGLE_QUOTE, "^(?:\'(?:[^\']*)\')+");
    cleri_t * r_double_quote = cleri_regex(CLERI_GID_R_DOUBLE_QUOTE, "^(?:\"(?:[^\"]*)\")+");
    cleri_t * thing_by_id = cleri_regex(CLERI_GID_THING_BY_ID, "^#[0-9]+");
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
    cleri_t * o_not = cleri_repeat(CLERI_GID_O_NOT, x_not, 0, 0);
    cleri_t * comments = cleri_repeat(CLERI_GID_COMMENTS, cleri_choice(
        CLERI_NONE,
        CLERI_FIRST_MATCH,
        2,
        cleri_regex(CLERI_NONE, "^(?s)//.*?(\\r?\\n|$)"),
        cleri_regex(CLERI_NONE, "^(?s)/\\\\*.*?\\\\*/")
    ), 0, 0);
    cleri_t * name = cleri_regex(CLERI_GID_NAME, "^[A-Za-z_][0-9A-Za-z_]*");
    cleri_t * var = cleri_regex(CLERI_GID_VAR, "^[A-Za-z_][0-9A-Za-z_]*");
    cleri_t * chain = cleri_ref();
    cleri_t * t_closure = cleri_sequence(
        CLERI_GID_T_CLOSURE,
        4,
        x_closure,
        cleri_list(CLERI_NONE, var, cleri_token(CLERI_NONE, ","), 0, 0, 1),
        cleri_token(CLERI_NONE, "|"),
        CLERI_THIS
    );
    cleri_t * thing = cleri_sequence(
        CLERI_GID_THING,
        3,
        x_thing,
        cleri_list(CLERI_NONE, cleri_sequence(
            CLERI_NONE,
            3,
            name,
            cleri_token(CLERI_NONE, ":"),
            CLERI_THIS
        ), cleri_token(CLERI_NONE, ","), 0, 0, 1),
        cleri_token(CLERI_NONE, "}")
    );
    cleri_t * array = cleri_sequence(
        CLERI_GID_ARRAY,
        3,
        x_array,
        cleri_list(CLERI_NONE, CLERI_THIS, cleri_token(CLERI_NONE, ","), 0, 0, 1),
        cleri_token(CLERI_NONE, "]")
    );
    cleri_t * function = cleri_sequence(
        CLERI_GID_FUNCTION,
        3,
        x_function,
        cleri_list(CLERI_NONE, CLERI_THIS, cleri_token(CLERI_NONE, ","), 0, 0, 1),
        cleri_token(CLERI_NONE, ")")
    );
    cleri_t * instance = cleri_dup(CLERI_GID_INSTANCE, thing);
    cleri_t * immutable = cleri_choice(
        CLERI_GID_IMMUTABLE,
        CLERI_FIRST_MATCH,
        8,
        t_false,
        t_nil,
        t_true,
        t_float,
        t_int,
        t_string,
        t_regex,
        t_closure
    );
    cleri_t * opr0_mul_div_mod = cleri_tokens(CLERI_GID_OPR0_MUL_DIV_MOD, "// * / %");
    cleri_t * opr1_add_sub = cleri_tokens(CLERI_GID_OPR1_ADD_SUB, "+ -");
    cleri_t * opr2_bitwise_and = cleri_tokens(CLERI_GID_OPR2_BITWISE_AND, "&");
    cleri_t * opr3_bitwise_xor = cleri_tokens(CLERI_GID_OPR3_BITWISE_XOR, "^");
    cleri_t * opr4_bitwise_or = cleri_tokens(CLERI_GID_OPR4_BITWISE_OR, "|");
    cleri_t * opr5_compare = cleri_tokens(CLERI_GID_OPR5_COMPARE, "== != <= >= < >");
    cleri_t * opr6_cmp_and = cleri_token(CLERI_GID_OPR6_CMP_AND, "&&");
    cleri_t * opr7_cmp_or = cleri_token(CLERI_GID_OPR7_CMP_OR, "||");
    cleri_t * opr8_ternary = cleri_sequence(
        CLERI_GID_OPR8_TERNARY,
        3,
        x_ternary,
        CLERI_THIS,
        cleri_token(CLERI_NONE, ":")
    );
    cleri_t * operations = cleri_sequence(
        CLERI_GID_OPERATIONS,
        3,
        CLERI_THIS,
        cleri_choice(
            CLERI_NONE,
            CLERI_FIRST_MATCH,
            9,
            opr8_ternary,
            opr7_cmp_or,
            opr6_cmp_and,
            opr5_compare,
            opr4_bitwise_or,
            opr3_bitwise_xor,
            opr2_bitwise_and,
            opr1_add_sub,
            opr0_mul_div_mod
        ),
        CLERI_THIS
    );
    cleri_t * assign = cleri_sequence(
        CLERI_GID_ASSIGN,
        2,
        x_assign,
        CLERI_THIS
    );
    cleri_t * name_opt_more = cleri_sequence(
        CLERI_GID_NAME_OPT_MORE,
        2,
        name,
        cleri_optional(CLERI_NONE, cleri_choice(
            CLERI_NONE,
            CLERI_FIRST_MATCH,
            2,
            function,
            assign
        ))
    );
    cleri_t * var_opt_more = cleri_sequence(
        CLERI_GID_VAR_OPT_MORE,
        2,
        var,
        cleri_optional(CLERI_NONE, cleri_choice(
            CLERI_NONE,
            CLERI_FIRST_MATCH,
            3,
            function,
            assign,
            instance
        ))
    );
    cleri_t * slice = cleri_list(CLERI_GID_SLICE, cleri_optional(CLERI_NONE, CLERI_THIS), cleri_token(CLERI_NONE, ":"), 0, 3, 0);
    cleri_t * index = cleri_repeat(CLERI_GID_INDEX, cleri_sequence(
        CLERI_NONE,
        4,
        x_index,
        slice,
        cleri_token(CLERI_NONE, "]"),
        cleri_optional(CLERI_NONE, cleri_sequence(
            CLERI_NONE,
            2,
            x_assign,
            CLERI_THIS
        ))
    ), 0, 0);
    cleri_t * block = cleri_sequence(
        CLERI_GID_BLOCK,
        4,
        x_block,
        comments,
        cleri_list(CLERI_NONE, CLERI_THIS, cleri_sequence(
            CLERI_NONE,
            2,
            cleri_token(CLERI_NONE, ";"),
            comments
        ), 1, 0, 1),
        cleri_token(CLERI_NONE, "}")
    );
    cleri_t * parenthesis = cleri_sequence(
        CLERI_GID_PARENTHESIS,
        3,
        x_parenthesis,
        CLERI_THIS,
        cleri_token(CLERI_NONE, ")")
    );
    cleri_t * expression = cleri_sequence(
        CLERI_GID_EXPRESSION,
        4,
        o_not,
        cleri_choice(
            CLERI_NONE,
            CLERI_FIRST_MATCH,
            8,
            chain,
            thing_by_id,
            immutable,
            var_opt_more,
            thing,
            array,
            block,
            parenthesis
        ),
        index,
        cleri_optional(CLERI_NONE, chain)
    );
    cleri_t * statement = cleri_prio(
        CLERI_GID_STATEMENT,
        2,
        expression,
        operations
    );
    cleri_t * statements = cleri_list(CLERI_GID_STATEMENTS, statement, cleri_sequence(
        CLERI_NONE,
        2,
        cleri_token(CLERI_NONE, ";"),
        comments
    ), 0, 0, 1);
    cleri_t * START = cleri_sequence(
        CLERI_GID_START,
        2,
        comments,
        statements
    );
    cleri_ref_set(chain, cleri_sequence(
        CLERI_GID_CHAIN,
        4,
        x_chain,
        name_opt_more,
        index,
        cleri_optional(CLERI_NONE, chain)
    ));

    cleri_grammar_t * grammar = cleri_grammar(START, "^[A-Za-z_][0-9A-Za-z_]*");

    return grammar;
}
