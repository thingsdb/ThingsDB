/*
 * langdef.h
 *
 * This grammar is generated using the Grammar.export_c() method and
 * should be used with the libcleri module.
 *
 * Source class: Definition
 * Created at: 2019-06-21 12:38:17
 */
#ifndef CLERI_EXPORT_LANGDEF_H_
#define CLERI_EXPORT_LANGDEF_H_

#include <cleri/cleri.h>

cleri_grammar_t * compile_langdef(void);

enum cleri_grammar_ids {
    CLERI_NONE,   // used for objects with no name
    CLERI_GID_ARRAY,
    CLERI_GID_ASSIGNMENT,
    CLERI_GID_CHAIN,
    CLERI_GID_CLOSURE,
    CLERI_GID_COMMENT,
    CLERI_GID_FUNCTION,
    CLERI_GID_INDEX,
    CLERI_GID_NAME,
    CLERI_GID_OPERATIONS,
    CLERI_GID_OPR0_MUL_DIV_MOD,
    CLERI_GID_OPR1_ADD_SUB,
    CLERI_GID_OPR2_BITWISE_AND,
    CLERI_GID_OPR3_BITWISE_XOR,
    CLERI_GID_OPR4_BITWISE_OR,
    CLERI_GID_OPR5_COMPARE,
    CLERI_GID_OPR6_CMP_AND,
    CLERI_GID_OPR7_CMP_OR,
    CLERI_GID_O_NOT,
    CLERI_GID_PRIMITIVES,
    CLERI_GID_R_DOUBLE_QUOTE,
    CLERI_GID_R_SINGLE_QUOTE,
    CLERI_GID_SCOPE,
    CLERI_GID_START,
    CLERI_GID_THING,
    CLERI_GID_TMP,
    CLERI_GID_TMP_ASSIGN,
    CLERI_GID_T_FALSE,
    CLERI_GID_T_FLOAT,
    CLERI_GID_T_INT,
    CLERI_GID_T_NIL,
    CLERI_GID_T_REGEX,
    CLERI_GID_T_STRING,
    CLERI_GID_T_TRUE,
    CLERI_END // can be used to get the enum length
};

#endif /* CLERI_EXPORT_LANGDEF_H_ */

