/*
 * ti/lookup.h
 */
#ifndef TI_LOOKUP_H_
#define TI_LOOKUP_H_

typedef struct ti_lookup_s ti_lookup_t;

#include <stdint.h>
#include <util/vec.h>

struct ti_lookup_s
{
    uint8_t n;                  /* equal or less than nodes->n */
    uint8_t r;                  /* equal to min(n, redundancy) */

    /* the three below are used for ti_lookup_node_is_ordered() */
    uint8_t * cache_;
    uint8_t * tmp_;
    uint64_t factorial_;

    vec_t * nodes_;     /* length is equal to r_ * n */
    uint64_t mask_[];
};

#include <ti/node.h>

int ti_lookup_create(uint8_t redundancy, const vec_t * vec_nodes);
void ti_lookup_destroy(void);
_Bool ti_lookup_node_has_id(ti_node_t * node, uint64_t id);
_Bool ti_lookup_node_is_ordered(uint8_t a, uint8_t b, uint64_t u);

#endif /* TI_LOOKUP_H_ */
