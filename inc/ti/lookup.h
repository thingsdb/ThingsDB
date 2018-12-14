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
    uint64_t factorial_;        /* only for order check */
    uint64_t mask_[];
};

#include <ti/node.h>

ti_lookup_t * ti_lookup_create(uint8_t n, uint8_t r);
void ti_lookup_destroy(ti_lookup_t * lookup);
_Bool ti_lookup_id_is_ordered(
        ti_lookup_t * lookup,
        uint8_t a,
        uint8_t b,
        uint64_t u);


#endif /* TI_LOOKUP_H_ */
