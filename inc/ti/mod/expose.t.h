/*
 * ti/mod/expose.t.h
 */
#ifndef TI_MOD_EXPOSE_T_H_
#define TI_MOD_EXPOSE_T_H_

#define TI_EXPOSE "module.json"

typedef struct ti_mod_expose_s ti_mod_expose_t;

#include <util/vec.h>

struct ti_mod_expose_s
{
    uint8_t * deep;
    _Bool * load;
    char * doc;
    vec_t * argmap;
    vec_t * defaults;
};

#endif  /* TI_MOD_EXPOSE_T_H_ */
