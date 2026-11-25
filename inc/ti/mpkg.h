/*
 * ti/mpkg.h
 */
#ifndef TI_MPKG_H_
#define TI_MPKG_H_

#include <stdint.h>
#include <stddef.h>
#include <ti/pkg.t.h>

typedef struct ti_mpkg_s ti_mpkg_t;

struct ti_mpkg_s
{
    int:32;
    uint32_t n;     /* size of data */
    uint16_t id;    /* id 0 is used for fire-and-forget packages */
    uint8_t tp;     /* see proto.h for protocol types */
    uint8_t ntp;    /* used as check-bit */
    unsigned char data[];
};

static inline ti_pkg_t * ti_mpkg_pkg(ti_mpkg_t * mpkg)
{
    return (ti_pkg_t *) (((char *) mpkg) + 4);
}

static inline void mpkg_init(ti_mpkg_t * mpkg, uint16_t id, uint8_t tp, size_t n)
{
    mpkg->id = id;
    mpkg->n = n;
    mpkg->tp = tp;
    mpkg->ntp = tp^0xff;
}

#endif /* TI_MPKG_H_ */
