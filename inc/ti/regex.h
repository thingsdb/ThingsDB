/*
 * ti/regex.h
 */
#ifndef TI_REGEX_H_
#define TI_REGEX_H_

#include <stddef.h>
#include <ti/raw.t.h>
#include <ti/regex.t.h>
#include <ex.h>

ti_regex_t * ti_regex_from_strn(const char * str, size_t n, ex_t * e);
ti_regex_t * ti_regex_from_str(const char * str);
ti_regex_t * ti_regex_create(ti_raw_t * pattern, ti_raw_t * flags, ex_t * e);
void ti_regex_destroy(ti_regex_t * regex);

#endif  /* TI_REGEX_H_ */
