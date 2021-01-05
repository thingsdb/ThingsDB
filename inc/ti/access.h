/*
 * access.h
 */
#ifndef TI_ACCESS_H_
#define TI_ACCESS_H_

#include <stdint.h>
#include <ti/user.h>
#include <util/vec.h>

int ti_access_grant(vec_t ** access, ti_user_t * user, uint64_t mask);
void ti_access_revoke(vec_t * access, ti_user_t * user, uint64_t mask);
_Bool ti_access_check(const vec_t * access, ti_user_t * user, uint64_t mask);
_Bool ti_access_check_or(const vec_t * access, ti_user_t * user, uint64_t mask);
int ti_access_check_err(
        const vec_t * access,
        ti_user_t * user,
        uint64_t mask,
        ex_t * e);
int ti_access_check_or_err(
        const vec_t * access,
        ti_user_t * user,
        uint64_t mask,
        ex_t * e);

#endif /* TI_ACCESS_H_ */
