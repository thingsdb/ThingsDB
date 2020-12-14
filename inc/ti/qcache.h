/*
 * ti/qcache.h
 */
#ifndef TI_QCACHE_H_
#define TI_QCACHE_H_

#include <inttypes.h>
#include <ti/query.t.h>

/*
 * Node info:
 *   - `cached_queries`
 *
 * Counters:
 *   - `queries_from_cache`
 *   - `unused_cache`
 *
 */


int ti_qcache_create(void);
void ti_qcache_destroy(void);
ti_query_t * ti_qcache_get_query(const char * str, size_t n, uint8_t flags);
void ti_qcache_return(ti_query_t * query);
void ti_qcache_cleanup(void);
_Bool ti_qcache_require_away(void);


#endif /* TI_QCACHE_H_ */
