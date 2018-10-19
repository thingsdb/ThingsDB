/*
 * auth.h
 */
#ifndef TI_AUTH_H_
#define TI_AUTH_H_

#define TI_AUTH_MASK_FULL ULLONG_MAX

typedef enum
{
    TI_AUTH_ACCESS=1,           /* root and database                        */
    TI_AUTH_READ,               /* root and database                        */
    TI_AUTH_MODIFY,             /* for root then means changing configuration
                                   like set_redundancy, add/replace nodes;
                                   for database this mean setting and removing
                                   things etc.
                                                                            */
    TI_AUTH_WATCH,              /* root and database                        */

    TI_AUTH_DB_CREATE,          /* root only                                */
    TI_AUTH_DB_DROP,            /* root and database                        */
    TI_AUTH_DB_CHANGE,          /* root and database                        */

    TI_AUTH_USER_CREATE,        /* root only                                */
    TI_AUTH_USER_DROP,          /* root only                                */
    TI_AUTH_USER_CHANGE,        /* root only                                */
    TI_AUTH_GRANT,              /* root and database                        */
    TI_AUTH_REVOKE,             /* root and database                        */
} ti_auth_e;

typedef struct ti_auth_s ti_auth_t;

#include <limits.h>
#include <stdint.h>
#include <ti/user.h>

ti_auth_t * ti_auth_new(ti_user_t * user, uint64_t mask);

struct ti_auth_s
{
    ti_user_t * user;  /* does not take a reference */
    uint64_t mask;
};

#endif /* TI_AUTH_H_ */
