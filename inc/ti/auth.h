/*
 * auth.h
 */
#ifndef TI_AUTH_H_
#define TI_AUTH_H_

#include <limits.h>
#include <stdint.h>

enum
{
    TI_AUTH_NO_ACCESS   =0,     /* default */
    TI_AUTH_READ        =1<<0,  /* root and collection, allow queries without
                                   events
                                */
    TI_AUTH_MODIFY      =1<<1,  /* root and collection, allow all queries
                                   including queries with events, for root
                                   anything `access` related is excluded and
                                   requires the `GRANT` flag
                                */
    TI_AUTH_WATCH       =1<<2,  /* (root? and) collection */
    TI_AUTH_GRANT       =1<<3,  /* root and collection, grant/revoke */
    TI_AUTH_MASK_FULL   =1<<0|  /* READ   */
                         1<<1|  /* MODIFY */
                         1<<2|  /* WATCH  */
                         1<<3,  /* GRANT  */
};


typedef struct ti_auth_s ti_auth_t;

#include <ti/user.h>


ti_auth_t * ti_auth_create(ti_user_t * user, uint16_t mask);
void ti_auth_destroy(ti_auth_t * auth);
const char * ti_auth_mask_to_str(uint16_t mask);


struct ti_auth_s
{
    ti_user_t * user;  /* does not take a reference */
    uint16_t mask;
};

#endif /* TI_AUTH_H_ */
