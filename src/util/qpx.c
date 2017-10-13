/*
 * qpx.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <string.h>
#include <util/qpx.h>

qp_res_t * qpx_map_get(qp_map_t * map, const char * key)
{
    for (size_t i = 0; map->n; i++)
    {
        qp_res_t * k = map->keys + i;
        if (k->tp == QP_RES_STR && strcmp(k->via.str, key) == 0)
        {
            return map->values + i;
        }
    }
    return NULL;
}

/*
 * Compare a raw object to a null terminated string.
 */
int qpx_raw_equal(qp_obj_t * obj, const char * s)
{
    for (size_t i = 0; i < obj->len; i++, s++)
    {
        if (*s != obj->via.raw[i] || !*s) return -1;
    }
    return 0;
}

qpx_packer_t * qpx_packer_create(size_t sz)
{
    qpx_packer_t * packer =  qp_packer_create(sizeof(rql_pkg_t) + sz);
    if (!packer) return NULL;
    packer->len = sizeof(rql_pkg_t);
    return packer;
}

rql_pkg_t * qpx_packer_pkg(qpx_packer_t * packer, uint8_t tp)
{
    rql_pkg_t * pkg = (rql_pkg_t *) packer->buffer;
    packer->buffer = NULL;
    qp_packer_destroy(packer);

    pkg->n = packer->len - sizeof(rql_pkg_t);
    pkg->tp = (uint8_t) tp;
    pkg->ntp = pkg->tp ^ 255;

    return pkg;
}
