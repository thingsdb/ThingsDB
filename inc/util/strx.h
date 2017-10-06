/*
 * strx.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_STRX_H_
#define RQL_STRX_H_

#include <stdbool.h>
#include <stddef.h>

void strx_upper_case(char * sptr);
void strx_lower_case(char * sptr);
void strx_replace_char(char * sptr, char orig, char repl);
int strx_replace_str(char * str, char * o, char * r, size_t n);
void strx_split_join(char * pt, char split_chr, char join_chr);

void strx_trim(char ** str, char chr);
bool strx_is_empty(const char * str);
bool strx_is_int(const char * str);
bool strx_is_float(const char * str);
bool strx_is_graph(const char * str);
double strx_to_double(const char * src, size_t len);
uint64_t strx_to_uint64(const char * src, size_t len);
char * strx_cat(const char * s1, const char * s2);

#endif /* RQL_STRX_H_ */
