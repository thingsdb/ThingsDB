/*
 * langdef.h
 *
 * This grammar is generated using the Grammar.export_c() method and
 * should be used with the libcleri module.
 *
 * Source class: Definition
 * Created at: 2018-10-21 16:31:53
 */
#ifndef CLERI_EXPORT_LANGDEF_H_
#define CLERI_EXPORT_LANGDEF_H_

#include <cleri/cleri.h>

cleri_grammar_t * compile_langdef(void);

enum cleri_grammar_ids {
    CLERI_NONE,   // used for objects with no name
    CLERI_GID_ACTION,
    CLERI_GID_ARRAY_IDX,
    CLERI_GID_AUTH_FLAGS,
    CLERI_GID_CHAIN,
    CLERI_GID_COMMENT,
    CLERI_GID_F_BLOB,
    CLERI_GID_F_CREATE,
    CLERI_GID_F_DELETE,
    CLERI_GID_F_DROP,
    CLERI_GID_F_FETCH,
    CLERI_GID_F_GRANT,
    CLERI_GID_F_MAP,
    CLERI_GID_F_RENAME,
    CLERI_GID_F_REVOKE,
    CLERI_GID_F_THING,
    CLERI_GID_IDENTIFIER,
    CLERI_GID_PRIMITIVES,
    CLERI_GID_R_COMMENT,
    CLERI_GID_R_DOUBLE_QUOTE_STR,
    CLERI_GID_R_FLOAT,
    CLERI_GID_R_INT,
    CLERI_GID_R_SINGLE_QUOTE_STR,
    CLERI_GID_SCOPE,
    CLERI_GID_START,
    CLERI_GID_STATEMENT,
    CLERI_GID_STRING,
    CLERI_GID_THING_ID,
    CLERI_END // can be used to get the enum length
};

#endif /* CLERI_EXPORT_LANGDEF_H_ */

