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
    TI_AUTH_QUERY       =1<<0,  /* allow queries without changes */
    TI_AUTH_CHANGE      =1<<1,  /* allow changes */
    TI_AUTH_GRANT       =1<<2,  /* grant/revoke */
    TI_AUTH_JOIN        =1<<3,  /* join/leave rooms */
    TI_AUTH_RUN         =1<<4,  /* run    */
    TI_AUTH_MASK_USER   =1<<0|  /* QUERY  */
                         1<<1|  /* CHANGE */
                         1<<3|  /* JOIN  */
                         1<<4,  /* RUN    */
    TI_AUTH_MASK_FULL   =1<<0|  /* QUERY  */
                         1<<1|  /* CHANGE */
                         1<<2|  /* GRANT  */
                         1<<3|  /* JOIN  */
                         1<<4,  /* RUN    */
};


typedef struct ti_auth_s ti_auth_t;

#include <ti/user.t.h>

ti_auth_t * ti_auth_create(ti_user_t * user, uint16_t mask);
void ti_auth_destroy(ti_auth_t * auth);
const char * ti_auth_mask_to_str(uint16_t mask);

struct ti_auth_s
{
    ti_user_t * user;       /* with reference */
    uint16_t mask;
};

#endif /* TI_AUTH_H_ */
