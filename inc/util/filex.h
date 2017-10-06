/*
 * filex.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_FILEX_H_
#define RQL_FILEX_H_

int filex_write(const char * fn, unsigned char * data, size_t n);
unsigned char * filex_read(const char * fn, ssize_t * size);



#endif /* RQL_FILEX_H_ */
