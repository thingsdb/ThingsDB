/*
 * qpx.h
 */
#ifndef TI_QPX_H_
#define TI_QPX_H_

#include <assert.h>
#include <qpack.h>
#include <ti/pkg.h>
#include <ti/raw.h>
#include <util/logger.h>

typedef qp_packer_t qpx_packer_t;

qp_res_t * qpx_map_get(const qp_map_t * map, const char * key);
char * qpx_obj_raw_to_str(const qp_obj_t * obj);
qpx_packer_t * qpx_packer_create(size_t alloc_size, size_t init_nest_size);
void qpx_packer_destroy(qpx_packer_t * xpkg);
ti_pkg_t * qpx_packer_pkg(qpx_packer_t * packer, uint8_t tp);
void qpx_unpacker_init(
        qp_unpacker_t * unpacker,
        const unsigned char * pt,
        size_t len);
char * qpx_raw_to_str(const qp_raw_t * raw);
void qpx__log_(
        const char * prelog,
        const uchar * data,
        size_t n,
        int log_level);
static inline void qpx_log(
        const char * prelog,
        const uchar * data,
        size_t n,
        int log_level);
static inline _Bool qpx_obj_endswith_raw(
        const qp_obj_t * obj,
        const ti_raw_t * raw);


static inline void qpx_log(
        const char * prelog,
        const uchar * data,
        size_t n,
        int log_level)
{
    if (Logger.level <= log_level)
        qpx__log_(prelog, data, n, log_level);
}

static inline _Bool qpx_obj_endswith_raw(
        const qp_obj_t * obj,
        const ti_raw_t * raw)
{
    return (
        obj->tp == QP_RAW &&
        obj->len >= raw->n &&
        memcmp(obj->via.raw + obj->len - raw->n, raw->data, raw->n) == 0
    );
}
#endif /* TI_QPX_H_ */
