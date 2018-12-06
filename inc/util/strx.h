/*
 * strx.h
 */
#ifndef TI_STRX_H_
#define TI_STRX_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
double strx_to_doublen(const char * str, size_t n);
uint64_t strx_to_uint64n(const char * str, size_t n);
int64_t strx_to_int64n(const char * str, size_t n);
/*
 * Overflow check:
 * int strx_to_int64n(int64_t * i, const char * str, size_t n);
 */
char * strx_cat(const char * s1, const char * s2);

#endif /* TI_STRX_H_ */
