/*
 * ti/pkg.t.h
 */
#ifndef TI_PKG_T_H_
#define TI_PKG_T_H_

/* 1GB */
#define TI_PKG_MAX_SIZE 1073741824

typedef struct ti_pkg_s ti_pkg_t;

#include <inttypes.h>

struct ti_pkg_s
{
    uint32_t n;     /* size of data */
    uint16_t id;    /* id 0 is used for fire-and-forget packages */
    uint8_t tp;     /* see proto.h for protocol types */
    uint8_t ntp;    /* used as check-bit */
    unsigned char data[];
};

#endif  /* TI_PKG_T_H_ */
