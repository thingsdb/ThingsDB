/*
 * ti/desired.h
 */
#ifndef TI_DESIRED_H_
#define TI_DESIRED_H_

typedef struct ti_desired_s ti_desired_t;

#include <ti/lookup.h>
#include <inttypes.h>

int ti_desired_create(void);
void ti_desired_destroy(void);
int ti_desired_init(void);
int ti_desired_set(uint8_t n, uint8_t r);

struct ti_desired_s
{
    uint8_t n;
    uint8_t r;
    ti_lookup_t * lookup;
};

#endif  /* TI_DESIRED_H_ */
