/*
 * langdef.h
 *
 * This grammar is generated using the Grammar.export_c() method and
 * should be used with the libcleri module.
 *
 * Source class: Definition
 * Created at: 2018-11-12 17:53:14
 */
#ifndef CLERI_EXPORT_LANGDEF_H_
#define CLERI_EXPORT_LANGDEF_H_

#include <cleri/cleri.h>

cleri_grammar_t * compile_langdef(void);

enum cleri_grammar_ids {
    CLERI_NONE,   // used for objects with no name
    CLERI_GID_ARRAY,
    CLERI_GID_ARROW,
    CLERI_GID_ASSIGNMENT,
    CLERI_GID_CHAIN,
    CLERI_GID_CMP_OPERATORS,
    CLERI_GID_COMMENT,
    CLERI_GID_COMPARE,
    CLERI_GID_FUNCTION,
    CLERI_GID_F_BLOB,
    CLERI_GID_F_DEL,
    CLERI_GID_F_ENDSWITH,
    CLERI_GID_F_FILTER,
    CLERI_GID_F_GET,
    CLERI_GID_F_ID,
    CLERI_GID_F_MAP,
    CLERI_GID_F_PUSH,
    CLERI_GID_F_REMOVE,
    CLERI_GID_F_RENAME,
    CLERI_GID_F_RET,
    CLERI_GID_F_SET,
    CLERI_GID_F_SPLICE,
    CLERI_GID_F_STARTSWITH,
    CLERI_GID_F_THING,
    CLERI_GID_F_UNSET,
    CLERI_GID_F_UNWATCH,
    CLERI_GID_F_WATCH,
    CLERI_GID_INDEX,
    CLERI_GID_NAME,
    CLERI_GID_O_NOT,
    CLERI_GID_PRIMITIVES,
    CLERI_GID_R_DOUBLE_QUOTE,
    CLERI_GID_R_SINGLE_QUOTE,
    CLERI_GID_SCOPE,
    CLERI_GID_START,
    CLERI_GID_THING,
    CLERI_GID_T_FALSE,
    CLERI_GID_T_FLOAT,
    CLERI_GID_T_INT,
    CLERI_GID_T_NIL,
    CLERI_GID_T_STRING,
    CLERI_GID_T_TRUE,
    CLERI_GID_T_UNDEFINED,
    CLERI_END // can be used to get the enum length
};

#endif /* CLERI_EXPORT_LANGDEF_H_ */

