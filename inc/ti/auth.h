/*
 * auth.h
 */
#ifndef TI_AUTH_H_
#define TI_AUTH_H_

#include <limits.h>
#include <stdint.h>

enum
{
    TI_AUTH_ACCESS      =1<<0,  /* root and database                         */
    TI_AUTH_READ        =1<<1,  /* root and database                         */
    TI_AUTH_MODIFY      =1<<2,  /* for root then means changing configuration
                                   like set_redundancy, add/replace nodes and
                                   changing database limits;
                                   for database this mean setting and removing
                                   things etc.
                                                                             */
    TI_AUTH_WATCH       =1<<3,  /* root and database                         */

    TI_AUTH_DB_CREATE   =1<<4,  /* root only                                 */
    TI_AUTH_DB_DROP     =1<<5,  /* root and database                         */
    TI_AUTH_DB_RENAME   =1<<6,  /* root and database                         */

    TI_AUTH_USER_CREATE =1<<7,  /* root only                                 */
    TI_AUTH_USER_DROP   =1<<8,  /* root only                                 */
    TI_AUTH_USER_CHANGE =1<<9,  /* root only                                 */
    TI_AUTH_USER_OTHER  =1<<10, /* when not set, user flags are forbidden
                                   on other account then your own.
                                                                             */
    TI_AUTH_GRANT       =1<<11, /* root and database                         */
    TI_AUTH_REVOKE      =1<<12, /* root and database                         */
};

enum
{
    TI_AUTH_MASK_READ=(TI_AUTH_ACCESS | TI_AUTH_READ | TI_AUTH_USER_CHANGE),
    TI_AUTH_MASK_MODIFY=(TI_AUTH_MASK_READ | TI_AUTH_MODIFY),
    TI_AUTH_MASK_FULL=UINT16_MAX
};

typedef struct ti_auth_s ti_auth_t;

#include <ti/user.h>


ti_auth_t * ti_auth_new(ti_user_t * user, uint16_t mask);
const char * ti_auth_mask_to_str(uint16_t mask);


struct ti_auth_s
{
    ti_user_t * user;  /* does not take a reference */
    uint16_t mask;
};

#endif /* TI_AUTH_H_ */
