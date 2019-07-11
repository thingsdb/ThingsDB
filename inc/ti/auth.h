/*
 * ti/auth.h
 */
#ifndef TI_AUTH_H_
#define TI_AUTH_H_

#include <limits.h>
#include <stdint.h>

enum
{
    TI_AUTH_NO_ACCESS   =0,     /* default */
    TI_AUTH_READ        =1<<0,  /* allow queries without events */
    TI_AUTH_MODIFY      =1<<1,  /* allow queries which require an event */
    TI_AUTH_WATCH       =1<<2,  /* watch/unwatch */
    TI_AUTH_CALL        =1<<3,  /* watch/unwatch */
    TI_AUTH_GRANT       =1<<4,  /* grant/revoke */
    TI_AUTH_MASK_FULL   =1<<0|  /* READ   */
                         1<<1|  /* MODIFY */
                         1<<2|  /* WATCH  */
                         1<<3|  /* CALL  */
                         1<<4,  /* GRANT  */
};


typedef struct ti_auth_s ti_auth_t;

#include <ti/user.h>


ti_auth_t * ti_auth_create(ti_user_t * user, uint16_t mask);
void ti_auth_destroy(ti_auth_t * auth);
const char * ti_auth_mask_to_str(uint16_t mask);


struct ti_auth_s
{
    ti_user_t * user;       /* with reference */
    uint16_t mask;
};

#endif /* TI_AUTH_H_ */
