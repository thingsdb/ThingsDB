/*
 * cryptx.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef CRYPTX_H_
#define CRYPTX_H_

#define CRYPTX_SZ 64
#define CRYPTX_SALT_SZ 10

void cryptx(const char * password, const char * salt, char * encrypted);
void cryptx_gen_salt(char * salt);

#endif /* CRYPTX_H_ */
