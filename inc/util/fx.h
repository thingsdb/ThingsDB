/*
 * filex.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_FILEX_H_
#define RQL_FILEX_H_

#include <stddef.h>
#include <sys/types.h>

int fx_write(const char * fn, unsigned char * data, size_t n);
unsigned char * fx_read(const char * fn, ssize_t * size);
_Bool fx_file_exist(const char * fn);
_Bool fx_is_dir(const char * path);
int fx_rmdir(const char * path);

#endif /* RQL_FILEX_H_ */
