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
    TI_AUTH_QUERY       =1<<0,  /* allow queries without events */
    TI_AUTH_EVENT       =1<<1,  /* allow events */
    TI_AUTH_GRANT       =1<<2,  /* grant/revoke */
    TI_AUTH_WATCH       =1<<3,  /* watch/unwatch */
    TI_AUTH_RUN         =1<<4,  /* run    */
    TI_AUTH_MASK_FULL   =1<<0|  /* QUERY  */
                         1<<1|  /* EVENT */
                         1<<2|  /* GRANT  */
                         1<<3|  /* WATCH  */
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
