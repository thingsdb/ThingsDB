/*
 * ti/whitelist.h
 */
#ifndef TI_WHITELIST_H_
#define TI_WHITELIST_H_

#include <stdlib.h>
#include <inttypes.h>
#include <ti/user.t.h>
#include <ex.h>

/* Do NOT CHANGE the order as this is used and stored in tasks */
enum
{
    TI_WHITELIST_ROOMS,             /* 0 */
    TI_WHITELIST_PROCEDURES,        /* 1 */
};

int ti_whitelist_add(vec_t ** whitelist, ti_val_t * val, ex_t * e);
int ti_whitelist_drop(vec_t ** whitelist, ti_val_t * val, ex_t * e);
int ti_whitelist_test(vec_t * whitelist, ti_name_t * name, ex_t * e);

#endif  /* TI_WHITELIST_H_ */
