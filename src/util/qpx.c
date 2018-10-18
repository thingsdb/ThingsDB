/*
 * qpx.c
 */
#include <assert.h>
#include <string.h>
#include <util/qpx.h>
#include <util/logger.h>

qp_res_t * qpx_map_get(const qp_map_t * map, const char * key)
{
    for (size_t i = 0; i < map->n; i++)
    {
        qp_res_t * k = map->keys + i;
        assert (k->tp == QP_RES_STR);
        if (strcmp(k->via.str, key) == 0)
        {
            return map->values + i;
        }
    }
    return NULL;
}

/*
 * Compare a raw object to a null terminated string.
 */
_Bool qpx_obj_eq_str(const qp_obj_t * obj, const char * s)
{
    if (obj->tp != QP_RAW) return 0;
    for (size_t i = 0; i < obj->len; i++, s++)
    {
        if (*s != obj->via.raw[i] || !*s) return 0;
    }
    return !*s;
}

qpx_packer_t * qpx_packer_create(size_t sz)
{
    qpx_packer_t * packer =  qp_packer_create(sizeof(ti_pkg_t) + sz);
    if (!packer) return NULL;
    packer->len = sizeof(ti_pkg_t);
    return packer;
}

void qpx_packer_destroy(qpx_packer_t * xpkg)
{
    if (!xpkg) return;
    qp_packer_destroy(xpkg);
}

ti_pkg_t * qpx_packer_pkg(qpx_packer_t * packer, uint8_t tp)
{
    ti_pkg_t * pkg = (ti_pkg_t *) packer->buffer;

    pkg->n = packer->len - sizeof(ti_pkg_t);
    pkg->tp = (uint8_t) tp;
    pkg->ntp = pkg->tp ^ 255;

    packer->buffer = NULL;
    qp_packer_destroy(packer);
    return pkg;
}

void qpx_unpacker_init(
        qp_unpacker_t * unpacker,
        const unsigned char * pt,
        size_t len)
{
    unpacker->flags = QP_UNPACK_FLAG_RAW | QP_UNPACK_FLAG_KEY_STR;
    unpacker->start = pt;
    unpacker->pt = pt;
    unpacker->end = pt + len;
}
