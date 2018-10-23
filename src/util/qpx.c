/*
 * qpx.c
 */
#include <assert.h>
#include <string.h>
#include <util/qpx.h>
#include <util/logger.h>
#include <stdlib.h>

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

char * qpx_obj_raw_to_str(const qp_obj_t * obj)
{
    assert (obj->tp == QP_RAW);
    char * str = malloc(obj->len + 1);
    if (!str)
        return NULL;
    memcpy(str, obj->via.raw, obj->len);
    str[obj->len] = '\0';
    return str;
}

qpx_packer_t * qpx_packer_create(size_t alloc_size, size_t init_nest_size)
{
    qpx_packer_t * packer = qp_packer_create2(
            sizeof(ti_pkg_t) + alloc_size,
            init_nest_size);
    if (!packer)
        return NULL;
    packer->len = sizeof(ti_pkg_t);
    return packer;
}

void qpx_packer_destroy(qpx_packer_t * xpkg)
{
    if (!xpkg)
        return;
    qp_packer_destroy(xpkg);
}

/*
 * If the input is valid, this function cannot fail
 */
ti_pkg_t * qpx_packer_pkg(qpx_packer_t * packer, uint8_t tp)
{
    ti_pkg_t * pkg = (ti_pkg_t *) packer->buffer;
    pkg->id = 0;
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
