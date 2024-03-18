/*
 * kwcache.h - holds keyword regular expression result while parsing.
 */
#ifndef CLERI_KWCACHE_H_
#define CLERI_KWCACHE_H_

#include <sys/types.h>
#include <cleri/cleri.h>
#include <cleri/parse.h>

/* typedefs */
typedef struct cleri_parse_s cleri_parse_t;

/* private functions */
uint8_t * cleri__kwcache_new(const char * str);
uint8_t cleri__kwcache_match(cleri_parse_t * pr, const char * str);
void cleri__kwcache_free(uint8_t * kwcache);

#endif /* CLERI_KWCACHE_H_ */

