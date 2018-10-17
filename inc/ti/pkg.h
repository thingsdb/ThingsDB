/*
 * pkg.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef TI_PKG_H_
#define TI_PKG_H_

#define TI_PKG_MAX_SIZE 209715200

typedef struct ti_pkg_s ti_pkg_t;

#include <stdint.h>
#include <util/ex.h>

ti_pkg_t * ti_pkg_new(
        uint16_t id,
        uint8_t tp,
        const unsigned char * data,
        uint32_t n);
ti_pkg_t * ti_pkg_err(uint16_t id, uint8_t tp, const char * errmsg);

struct ti_pkg_s
{
    uint32_t n;     /* size of data */
    uint16_t id;
    uint8_t tp;
    uint8_t ntp;    /* used as check-bit */
    unsigned char data[];
};

/* setting ntp is to avoid ~ unsigned warn */
#define ti_pkg_check(pkg__) (\
        ((pkg__)->tp == ((pkg__)->ntp ^= 255)) && \
        ((pkg__)->tp != ((pkg__)->ntp ^= 255)) && \
        (pkg__)->n <= TI_PKG_MAX_SIZE)

#endif /* TI_PKG_H_ */
