/*
 * langdef.c
 *
 * This grammar is generated using the Grammar.export_c() method and
 * should be used with the libcleri module.
 *
 * Source class: Definition
 * Created at: 2019-04-11 12:05:32
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
    cleri_t * f_blob = cleri_keyword(CLERI_GID_F_BLOB, "blob", CLERI_CASE_SENSITIVE);
    cleri_t * f_endswith = cleri_keyword(CLERI_GID_F_ENDSWITH, "endswith", CLERI_CASE_SENSITIVE);
    cleri_t * f_filter = cleri_keyword(CLERI_GID_F_FILTER, "filter", CLERI_CASE_SENSITIVE);
    cleri_t * f_findindex = cleri_keyword(CLERI_GID_F_FINDINDEX, "findindex", CLERI_CASE_SENSITIVE);
    cleri_t * f_find = cleri_keyword(CLERI_GID_F_FIND, "find", CLERI_CASE_SENSITIVE);
    cleri_t * f_hasprop = cleri_keyword(CLERI_GID_F_HASPROP, "hasprop", CLERI_CASE_SENSITIVE);
    cleri_t * f_id = cleri_keyword(CLERI_GID_F_ID, "id", CLERI_CASE_SENSITIVE);
    cleri_t * f_indexof = cleri_keyword(CLERI_GID_F_INDEXOF, "indexof", CLERI_CASE_SENSITIVE);
    cleri_t * f_int = cleri_keyword(CLERI_GID_F_INT, "int", CLERI_CASE_SENSITIVE);
    cleri_t * f_isarray = cleri_keyword(CLERI_GID_F_ISARRAY, "isarray", CLERI_CASE_SENSITIVE);
    cleri_t * f_isascii = cleri_keyword(CLERI_GID_F_ISASCII, "isascii", CLERI_CASE_SENSITIVE);
    cleri_t * f_isbool = cleri_keyword(CLERI_GID_F_ISBOOL, "isbool", CLERI_CASE_SENSITIVE);
    cleri_t * f_isfloat = cleri_keyword(CLERI_GID_F_ISFLOAT, "isfloat", CLERI_CASE_SENSITIVE);
    cleri_t * f_isinf = cleri_keyword(CLERI_GID_F_ISINF, "isinf", CLERI_CASE_SENSITIVE);
    cleri_t * f_isint = cleri_keyword(CLERI_GID_F_ISINT, "isint", CLERI_CASE_SENSITIVE);
    cleri_t * f_islist = cleri_keyword(CLERI_GID_F_ISLIST, "islist", CLERI_CASE_SENSITIVE);
    cleri_t * f_isnan = cleri_keyword(CLERI_GID_F_ISNAN, "isnan", CLERI_CASE_SENSITIVE);
    cleri_t * f_israw = cleri_keyword(CLERI_GID_F_ISRAW, "israw", CLERI_CASE_SENSITIVE);
    cleri_t * f_isstr = cleri_keyword(CLERI_GID_F_ISSTR, "isstr", CLERI_CASE_SENSITIVE);
    cleri_t * f_istuple = cleri_keyword(CLERI_GID_F_ISTUPLE, "istuple", CLERI_CASE_SENSITIVE);
    cleri_t * f_isutf8 = cleri_keyword(CLERI_GID_F_ISUTF8, "isutf8", CLERI_CASE_SENSITIVE);
    cleri_t * f_len = cleri_keyword(CLERI_GID_F_LEN, "len", CLERI_CASE_SENSITIVE);
    cleri_t * f_lower = cleri_keyword(CLERI_GID_F_LOWER, "lower", CLERI_CASE_SENSITIVE);
    cleri_t * f_map = cleri_keyword(CLERI_GID_F_MAP, "map", CLERI_CASE_SENSITIVE);
    cleri_t * f_now = cleri_keyword(CLERI_GID_F_NOW, "now", CLERI_CASE_SENSITIVE);
    cleri_t * f_refs = cleri_keyword(CLERI_GID_F_REFS, "refs", CLERI_CASE_SENSITIVE);
    cleri_t * f_remove = cleri_keyword(CLERI_GID_F_REMOVE, "remove", CLERI_CASE_SENSITIVE);
    cleri_t * f_ret = cleri_keyword(CLERI_GID_F_RET, "ret", CLERI_CASE_SENSITIVE);
    cleri_t * f_startswith = cleri_keyword(CLERI_GID_F_STARTSWITH, "startswith", CLERI_CASE_SENSITIVE);
    cleri_t * f_str = cleri_keyword(CLERI_GID_F_STR, "str", CLERI_CASE_SENSITIVE);
    cleri_t * f_t = cleri_keyword(CLERI_GID_F_T, "t", CLERI_CASE_SENSITIVE);
    cleri_t * f_test = cleri_keyword(CLERI_GID_F_TEST, "test", CLERI_CASE_SENSITIVE);
    cleri_t * f_try = cleri_keyword(CLERI_GID_F_TRY, "try", CLERI_CASE_SENSITIVE);
    cleri_t * f_upper = cleri_keyword(CLERI_GID_F_UPPER, "upper", CLERI_CASE_SENSITIVE);
    cleri_t * f_del = cleri_keyword(CLERI_GID_F_DEL, "del", CLERI_CASE_SENSITIVE);
    cleri_t * f_pop = cleri_keyword(CLERI_GID_F_POP, "pop", CLERI_CASE_SENSITIVE);
    cleri_t * f_push = cleri_keyword(CLERI_GID_F_PUSH, "push", CLERI_CASE_SENSITIVE);
    cleri_t * f_rename = cleri_keyword(CLERI_GID_F_RENAME, "rename", CLERI_CASE_SENSITIVE);
    cleri_t * f_splice = cleri_keyword(CLERI_GID_F_SPLICE, "splice", CLERI_CASE_SENSITIVE);
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
        cleri_choice(
            CLERI_NONE,
            CLERI_FIRST_MATCH,
            40,
            f_blob,
            f_endswith,
            f_filter,
            f_findindex,
            f_find,
            f_hasprop,
            f_id,
            f_indexof,
            f_int,
            f_isarray,
            f_isascii,
            f_isbool,
            f_isfloat,
            f_isinf,
            f_isint,
            f_islist,
            f_isnan,
            f_israw,
            f_isstr,
            f_istuple,
            f_isutf8,
            f_len,
            f_lower,
            f_map,
            f_now,
            f_refs,
            f_remove,
            f_ret,
            f_startswith,
            f_str,
            f_t,
            f_test,
            f_try,
            f_upper,
            f_del,
            f_pop,
            f_push,
            f_rename,
            f_splice,
            name
        ),
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
    cleri_t * START = cleri_sequence(
        CLERI_GID_START,
        2,
        comment,
        cleri_list(CLERI_NONE, scope, cleri_sequence(
            CLERI_NONE,
            2,
            cleri_token(CLERI_NONE, ";"),
            comment
        ), 0, 0, 1)
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
