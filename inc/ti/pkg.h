/*
 * ti/pkg.h
 */
#ifndef TI_PKG_H_
#define TI_PKG_H_

#define TI_PKG_MAX_SIZE 209715200

typedef struct ti_pkg_s ti_pkg_t;

#include <stdint.h>
#include <stddef.h>
#include <ex.h>

ti_pkg_t * ti_pkg_new(
        uint16_t id,
        uint8_t tp,
        const unsigned char * data,
        uint32_t n);
void pkg_init(ti_pkg_t * pkg, uint16_t id, uint8_t tp, size_t total_n);
ti_pkg_t * ti_pkg_dup(ti_pkg_t * pkg);
ti_pkg_t * ti_pkg_client_err(uint16_t id, ex_t * e);
void ti_pkg_client_err_to_e(ex_t * e, ti_pkg_t * pkg);
ti_pkg_t * ti_pkg_node_err(uint16_t id, ex_t * e);
void ti_pkg_log(ti_pkg_t * pkg);
void ti_pkg_set_tp(ti_pkg_t * pkg, uint8_t tp);
static inline size_t ti_pkg_sz(ti_pkg_t * pkg);

struct ti_pkg_s
{
    uint32_t n;     /* size of data */
    uint16_t id;    /* id 0 is used for fire-and-forget packages */
    uint8_t tp;     /* see proto.h for protocol types */
    uint8_t ntp;    /* used as check-bit */
    unsigned char data[];
};

/* setting ntp is to avoid ~ unsigned warn */
#define ti_pkg_check(pkg__) (\
        (pkg__)->tp == ((pkg__)->ntp ^= 0xff) && \
        (pkg__)->tp != ((pkg__)->ntp ^= 0xff) && \
        (pkg__)->n <= TI_PKG_MAX_SIZE)

/* return total package size, header + data size */
static inline size_t ti_pkg_sz(ti_pkg_t * pkg)
{
    return pkg->n + sizeof(ti_pkg_t);
}

#endif /* TI_PKG_H_ */
