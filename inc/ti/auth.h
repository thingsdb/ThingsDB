/*
 * auth.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef TI_AUTH_H_
#define TI_AUTH_H_

#define TI_AUTH_MASK_FULL ULLONG_MAX

typedef enum
{
    /* 0.. 60 are reserved for task authorization */
    TI_AUTH_READ=60
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
