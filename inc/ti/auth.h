/*
 * auth.h
 */
#ifndef TI_AUTH_H_
#define TI_AUTH_H_

typedef enum
{
    TI_AUTH_ACCESS      =0,     /* root and database                         */
    TI_AUTH_READ        =1,     /* root and database                         */
    TI_AUTH_MODIFY      =2,     /* for root then means changing configuration
                                   like set_redundancy, add/replace nodes;
                                   for database this mean setting and removing
                                   things etc.
                                                                             */
    TI_AUTH_WATCH       =3,     /* root and database                         */

    TI_AUTH_DB_CREATE   =4,     /* root only                                 */
    TI_AUTH_DB_DROP     =5,     /* root and database                         */
    TI_AUTH_DB_CHANGE   =6,     /* root and database                         */

    TI_AUTH_USER_CREATE =6,     /* root only                                 */
    TI_AUTH_USER_DROP   =7,     /* root only                                 */
    TI_AUTH_USER_CHANGE =8,     /* root only                                 */
    TI_AUTH_USER_OTHER  =9,     /* when set, user flags are ONLY applicable
                                   to your own account
                                                                             */
    TI_AUTH_GRANT       =10,    /* root and database                         */
    TI_AUTH_REVOKE      =11,    /* root and database                         */
} ti_auth_e;

static const int ti_auth_profile_read = (
        1 << TI_AUTH_ACCESS |
        1 << TI_AUTH_READ
);

static const int ti_auth_profile_modify = (
        1 << TI_AUTH_ACCESS     |
        1 << TI_AUTH_READ       |
        1 << TI_AUTH_MODIFY  |
        1 << TI_AUTH_MODIFY

);

static const int ti_auth_profile_full = (
        1 << TI_AUTH_ACCESS |
        1 << TI_AUTH_READ
);

typedef struct ti_auth_s ti_auth_t;

#include <limits.h>
#include <stdint.h>
#include <ti/user.h>

#define TI_AUTH_FULL UINT16_MAX

ti_auth_t * ti_auth_new(ti_user_t * user, uint64_t mask);

struct ti_auth_s
{
    ti_user_t * user;  /* does not take a reference */
    uint16_t mask;
};

#endif /* TI_AUTH_H_ */
