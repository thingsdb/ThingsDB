/*
 * ti/lookup.h
 */
#ifndef TI_LOOKUP_H_
#define TI_LOOKUP_H_

#define LOOKUP_SIZE 64

typedef struct ti_lookup_s ti_lookup_t;

#include <stdint.h>
#include <util/vec.h>

struct ti_lookup_s
{
    uint8_t n;                  /* equal or less than nodes->n */
    uint64_t factorial_;        /* only for order check */
};

ti_lookup_t * ti_lookup_create(uint8_t n);
void ti_lookup_destroy(ti_lookup_t * lookup);
_Bool ti_lookup_id_is_ordered(
        ti_lookup_t * lookup,
        uint8_t a,
        uint8_t b,
        uint64_t u);

#endif /* TI_LOOKUP_H_ */
