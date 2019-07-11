/*
 * ti/ncache.h
 */
#ifndef TI_NCACHE_H_
#define TI_NCACHE_H_

typedef struct ti_ncache_s ti_ncache_t;

#include <cleri/cleri.h>
#include <ti/ex.h>
#include <util/vec.h>

ti_ncache_t * ti_ncache_create(char * query, size_t n);
void ti_ncache_destroy(ti_ncache_t * ncache);
int ti_ncache_gen_primitives(vec_t * vcache, cleri_node_t * node, ex_t * e);

struct ti_ncache_s
{
    char * query;
    vec_t * nd_val_cache;
};


#endif /* TI_NCACHE_H_ */
