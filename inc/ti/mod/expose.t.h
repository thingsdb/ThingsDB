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
    uint8_t * deep;             /* NULL or 0..127 */
    _Bool * load;               /* NULL or true/false */
    char * doc;                 /* NULL or string */
    vec_t * argmap;             /* ti_item_t */
    vec_t * defaults;           /* ti_item_t */
};

#endif  /* TI_MOD_EXPOSE_T_H_ */
