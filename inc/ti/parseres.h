/*
 * ti/parseres.h
 */
#ifndef TI_PARSERES_H_
#define TI_PARSERES_H_

#include <cleri/cleri.h>
#include <ex.h>

int ti_parseres_syntax_err(cleri_parse_t *pr, ex_t *e);
int ti_parseres_syntax_err_tpl(cleri_parse_t *pr, ex_t *e);

#endif /* TI_PARSERES_H_ */
