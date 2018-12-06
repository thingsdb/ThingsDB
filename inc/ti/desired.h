/*
 * ti/desired.h
 */
#ifndef TI_DESIRED_H_
#define TI_DESIRED_H_

typedef struct ti_desired_s ti_desired_t;

#include <ti/lookup.h>

struct ti_desired_s
{
    ti_lookup_t * lookup;
};

#endif  /* TI_DESIRED_H_ */
