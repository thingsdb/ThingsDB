/*
 * pkg.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_PKG_H_
#define RQL_PKG_H_

#define RQL_PKG_MAX_SIZE 209715200

typedef struct rql_pkg_s rql_pkg_t;

#include <inttypes.h>
#include <util/ex.h>

rql_pkg_t * rql_pkg_new(uint8_t tp, const unsigned char * data, uint32_t n);
rql_pkg_t * rql_pkg_e(ex_t * e, uint16_t id);

struct rql_pkg_s
{
    uint32_t n;     /* size of data */
    uint16_t id;
    uint8_t tp;
    uint8_t ntp;    /* used as check-bit */
    unsigned char data[];
};

/* setting ntp is to avoid ~ unsigned warn */
#define rql_pkg_check(pkg__) (\
        ((pkg__)->tp == ((pkg__)->ntp ^= 255)) && \
        ((pkg__)->tp != ((pkg__)->ntp ^= 255)) && \
        (pkg__)->n <= RQL_PKG_MAX_SIZE)

#endif /* RQL_PKG_H_ */
