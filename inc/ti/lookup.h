/*
 * lookup.h
 */
#ifndef TI_LOOKUP_H_
#define TI_LOOKUP_H_

typedef struct ti_lookup_s ti_lookup_t;

#include <stdint.h>
#include <util/vec.h>
#include <ti/node.h>
ti_lookup_t * ti_lookup_create(
        uint8_t n,
        uint8_t redundancy,
        const vec_t * nodes);
void ti_lookup_destroy(ti_lookup_t * lookup);
_Bool ti_lookup_node_has_id(
        ti_lookup_t * lookup,
        ti_node_t * node,
        uint64_t id);

struct ti_lookup_s
{
    uint8_t n_;         /* equal to nodes->n */
    uint8_t r_;         /* equal to min(n, redundancy) */
    vec_t * nodes_;     /* length is equal to r_ * n */
    uint64_t mask_[];
};

#endif /* TI_LOOKUP_H_ */
