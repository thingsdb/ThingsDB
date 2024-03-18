/*
 * util/osarch.h
 */
#ifndef UTIL_OSARCH_H_
#define UTIL_OSARCH_H_

void osarch_init(void);                 /* initialize */
const char * osarch_get_os(void);       /* example: linux */
const char * osarch_get_arch(void);     /* example: amd64 */
const char * osarch_get(void);          /* example: linux/amd64 */

#endif  /* UTIL_OSARCH_H_ */
