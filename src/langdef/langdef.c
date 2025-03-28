/*
 * langdef.c
 *
 * This grammar is generated using the Grammar.export_c() method and
 * should be used with the libcleri module.
 *
 * Source class: LangDef
 * Created at: 2024-02-11 20:55:25
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
    cleri_t * x_chain = cleri_tokens(CLERI_GID_X_CHAIN, ".. .");
    cleri_t * x_closure = cleri_token(CLERI_GID_X_CLOSURE, "|");
    cleri_t * x_function = cleri_token(CLERI_GID_X_FUNCTION, "(");
    cleri_t * x_index = cleri_token(CLERI_GID_X_INDEX, "[");
    cleri_t * x_parenthesis = cleri_token(CLERI_GID_X_PARENTHESIS, "(");
    cleri_t * x_preopr = cleri_regex(CLERI_GID_X_PREOPR, "^(\\s*~)*(\\s*!|\\s*[\\-+](?=[^0-9]))*");
    cleri_t * x_ternary = cleri_token(CLERI_GID_X_TERNARY, "?");
    cleri_t * x_thing = cleri_token(CLERI_GID_X_THING, "{");
    cleri_t * x_template = cleri_token(CLERI_GID_X_TEMPLATE, "`");
    cleri_t * template = cleri_sequence(
        CLERI_GID_TEMPLATE,
        3,
        x_template,
        cleri_repeat(CLERI_NONE, cleri_choice(
            CLERI_NONE,
            CLERI_FIRST_MATCH,
            2,
            cleri_regex(CLERI_NONE, "^([^`{}]|``|{{|}})+"),
            cleri_sequence(
                CLERI_NONE,
                3,
                cleri_token(CLERI_NONE, "{"),
                CLERI_THIS,
                cleri_token(CLERI_NONE, "}")
            )
        ), 0, 0),
        x_template
    );
    cleri_t * t_false = cleri_keyword(CLERI_GID_T_FALSE, "false", CLERI_CASE_SENSITIVE);
    cleri_t * t_float = cleri_regex(CLERI_GID_T_FLOAT, "^[-+]?(inf|nan|[0-9]*\\.[0-9]+(e[+-][0-9]+)?)(?![0-9A-Za-z_\\.])");
    cleri_t * t_int = cleri_regex(CLERI_GID_T_INT, "^[-+]?((0b[01]+)|(0o[0-8]+)|(0x[0-9a-fA-F]+)|([0-9]+))(?![0-9A-Za-z_\\.])");
    cleri_t * t_nil = cleri_keyword(CLERI_GID_T_NIL, "nil", CLERI_CASE_SENSITIVE);
    cleri_t * t_regex = cleri_regex(CLERI_GID_T_REGEX, "^/((?:.(?!(?<![\\\\])/))*.?)/[a-z]*");
    cleri_t * t_string = cleri_regex(CLERI_GID_T_STRING, "^(((?:\'(?:[^\']*)\')+)|((?:\"(?:[^\"]*)\")+))");
    cleri_t * t_true = cleri_keyword(CLERI_GID_T_TRUE, "true", CLERI_CASE_SENSITIVE);
    cleri_t * name = cleri_regex(CLERI_GID_NAME, "^[A-Za-z_][0-9A-Za-z_]{0,254}(?![0-9A-Za-z_])");
    cleri_t * var = cleri_regex(CLERI_GID_VAR, "^[A-Za-z_][0-9A-Za-z_]{0,254}(?![0-9A-Za-z_])");
    cleri_t * chain = cleri_ref();
    cleri_t * closure = cleri_sequence(
        CLERI_GID_CLOSURE,
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
            cleri_optional(CLERI_NONE, CLERI_THIS)
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
    cleri_t * enum_ = cleri_sequence(
        CLERI_GID_ENUM_,
        3,
        x_thing,
        cleri_choice(
            CLERI_NONE,
            CLERI_FIRST_MATCH,
            2,
            name,
            closure
        ),
        cleri_token(CLERI_NONE, "}")
    );
    cleri_t * opr0_mul_div_mod = cleri_tokens(CLERI_GID_OPR0_MUL_DIV_MOD, "* / %");
    cleri_t * opr1_add_sub = cleri_tokens(CLERI_GID_OPR1_ADD_SUB, "+ -");
    cleri_t * opr2_bitwise_shift = cleri_tokens(CLERI_GID_OPR2_BITWISE_SHIFT, "<< >>");
    cleri_t * opr3_bitwise_and = cleri_tokens(CLERI_GID_OPR3_BITWISE_AND, "&");
    cleri_t * opr4_bitwise_xor = cleri_tokens(CLERI_GID_OPR4_BITWISE_XOR, "^");
    cleri_t * opr5_bitwise_or = cleri_tokens(CLERI_GID_OPR5_BITWISE_OR, "|");
    cleri_t * opr6_compare = cleri_tokens(CLERI_GID_OPR6_COMPARE, "== != <= >= < >");
    cleri_t * opr7_cmp_and = cleri_token(CLERI_GID_OPR7_CMP_AND, "&&");
    cleri_t * opr8_cmp_or = cleri_token(CLERI_GID_OPR8_CMP_OR, "||");
    cleri_t * opr9_ternary = cleri_sequence(
        CLERI_GID_OPR9_TERNARY,
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
            10,
            opr9_ternary,
            opr8_cmp_or,
            opr7_cmp_and,
            opr2_bitwise_shift,
            opr6_compare,
            opr5_bitwise_or,
            opr4_bitwise_xor,
            opr3_bitwise_and,
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
            4,
            function,
            assign,
            instance,
            enum_
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
        3,
        x_block,
        cleri_list(CLERI_NONE, CLERI_THIS, cleri_repeat(CLERI_NONE, cleri_token(CLERI_NONE, ";"), 1, 0), 1, 0, 1),
        cleri_token(CLERI_NONE, "}")
    );
    cleri_t * parenthesis = cleri_sequence(
        CLERI_GID_PARENTHESIS,
        3,
        x_parenthesis,
        CLERI_THIS,
        cleri_token(CLERI_NONE, ")")
    );
    cleri_t * k_if = cleri_keyword(CLERI_GID_K_IF, "if", CLERI_CASE_SENSITIVE);
    cleri_t * k_else = cleri_keyword(CLERI_GID_K_ELSE, "else", CLERI_CASE_SENSITIVE);
    cleri_t * k_return = cleri_keyword(CLERI_GID_K_RETURN, "return", CLERI_CASE_SENSITIVE);
    cleri_t * k_for = cleri_keyword(CLERI_GID_K_FOR, "for", CLERI_CASE_SENSITIVE);
    cleri_t * k_in = cleri_keyword(CLERI_GID_K_IN, "in", CLERI_CASE_SENSITIVE);
    cleri_t * k_continue = cleri_keyword(CLERI_GID_K_CONTINUE, "continue", CLERI_CASE_SENSITIVE);
    cleri_t * k_break = cleri_keyword(CLERI_GID_K_BREAK, "break", CLERI_CASE_SENSITIVE);
    cleri_t * if_statement = cleri_sequence(
        CLERI_GID_IF_STATEMENT,
        6,
        k_if,
        cleri_token(CLERI_NONE, "("),
        CLERI_THIS,
        cleri_token(CLERI_NONE, ")"),
        CLERI_THIS,
        cleri_optional(CLERI_NONE, cleri_sequence(
            CLERI_NONE,
            2,
            k_else,
            CLERI_THIS
        ))
    );
    cleri_t * return_statement = cleri_sequence(
        CLERI_GID_RETURN_STATEMENT,
        2,
        k_return,
        cleri_list(CLERI_NONE, CLERI_THIS, cleri_token(CLERI_NONE, ","), 0, 3, 0)
    );
    cleri_t * for_statement = cleri_sequence(
        CLERI_GID_FOR_STATEMENT,
        7,
        k_for,
        cleri_token(CLERI_NONE, "("),
        cleri_list(CLERI_NONE, var, cleri_token(CLERI_NONE, ","), 1, 0, 0),
        k_in,
        CLERI_THIS,
        cleri_token(CLERI_NONE, ")"),
        CLERI_THIS
    );
    cleri_t * expression = cleri_sequence(
        CLERI_GID_EXPRESSION,
        4,
        x_preopr,
        cleri_choice(
            CLERI_NONE,
            CLERI_FIRST_MATCH,
            13,
            chain,
            t_false,
            t_nil,
            t_true,
            t_float,
            t_int,
            t_string,
            t_regex,
            template,
            var_opt_more,
            thing,
            array,
            parenthesis
        ),
        index,
        cleri_optional(CLERI_NONE, chain)
    );
    cleri_t * statement = cleri_prio(
        CLERI_GID_STATEMENT,
        4,
        k_continue,
        k_break,
        cleri_choice(
            CLERI_NONE,
            CLERI_FIRST_MATCH,
            6,
            if_statement,
            return_statement,
            for_statement,
            closure,
            expression,
            block
        ),
        operations
    );
    cleri_t * START = cleri_list(CLERI_GID_START, statement, cleri_repeat(CLERI_NONE, cleri_token(CLERI_NONE, ";"), 1, 0), 0, 0, 1);
    cleri_ref_set(chain, cleri_sequence(
        CLERI_GID_CHAIN,
        4,
        x_chain,
        name_opt_more,
        index,
        cleri_optional(CLERI_NONE, chain)
    ));

    cleri_grammar_t * grammar = cleri_grammar2(
            START,
            "^[A-Za-z_][0-9A-Za-z_]{0,254}(?![0-9A-Za-z_])",
            "("
            "(\\s+)|"
            "((?s)\\/\\/.*?(\\r?\\n|$))|"
            "((?s)\\/\\*.*?\\*\\/)"
            ")*");
    return grammar;
}
