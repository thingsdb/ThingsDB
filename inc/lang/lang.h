/*
 * lang/lang.h
 *
 * This grammar is generated using the Grammar.export_c() method and
 * should be used with the libcleri module.
 *
 * Source class: RqlLang
 * Created at: 2017-09-29 22:49:23
 */
#ifndef CLERI_EXPORT_LANG_LANG_H_
#define CLERI_EXPORT_LANG_LANG_H_

#include <cleri/cleri.h>

cleri_grammar_t * compile_grammar(void);

enum cleri_grammar_ids {
    CLERI_NONE,   // used for objects with no name
    CLERI_GID_COMPARE_OPR,
    CLERI_GID_ELEM_ID,
    CLERI_GID_FILTER,
    CLERI_GID_KIND,
    CLERI_GID_MARK,
    CLERI_GID_PROP,
    CLERI_GID_R_FLOAT,
    CLERI_GID_R_INT,
    CLERI_GID_R_NAME,
    CLERI_GID_R_STR,
    CLERI_GID_R_UINT,
    CLERI_GID_SELECTOR,
    CLERI_GID_START,
    CLERI_GID_T_AND,
    CLERI_GID_T_OR,
    CLERI_END // can be used to get the enum length
};

#endif /* CLERI_EXPORT_LANG_LANG_H_ */

