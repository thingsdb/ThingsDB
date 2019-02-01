/*
 * strx.h
 */
#ifndef TI_STRX_H_
#define TI_STRX_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <lib/simdutf8check.h>


void strx_upper_case(char * sptr);
void strx_lower_case(char * sptr);
void strx_replace_char(char * sptr, char orig, char repl);
int strx_replace_str(char * str, char * o, char * r, size_t n);
void strx_split_join(char * pt, char split_chr, char join_chr);

void strx_trim(char ** str, char chr);
_Bool strx_is_empty(const char * str);
_Bool strx_is_int(const char * str);
_Bool strx_is_float(const char * str);
_Bool strx_is_graph(const char * str);
_Bool strx_is_graphn(const char * str, size_t n);
_Bool strx_is_asciin(const char * str, size_t n);
static inline _Bool strx_is_utf8n(const char * str, size_t n);
double strx_to_double(const char * str);
int64_t strx_to_int64(const char * str);
const char * strx_from_double(const double d, size_t * n);  /* not thread safe */
const char * strx_from_int64(const int64_t i, size_t * n);  /* not thread safe */
char * strx_cat(const char * s1, const char * s2);

static inline _Bool strx_is_utf8n(const char * str, size_t n)
{
    return validate_utf8_fast(str, n);
}
#endif /* TI_STRX_H_ */
