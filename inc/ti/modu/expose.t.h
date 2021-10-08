/*
 * ti/modu/expose.t.h
 */
#ifndef TI_MODU_EXPOSE_T_H_
#define TI_MODU_EXPOSE_T_H_

#define TI_EXPOSE "module.json"

typedef struct ti_modu_expose_s ti_modu_expose_t;

#include <util/vec.h>

struct ti_modu_expose_s
{
    uint8_t * deep;             /* NULL or 0..127 */
    _Bool * load;               /* NULL or true/false */
    char * doc;                 /* NULL or string */
    vec_t * argmap;             /* ti_item_t */
    vec_t * defaults;           /* ti_item_t */
};

#endif  /* TI_MODU_EXPOSE_T_H_ */
