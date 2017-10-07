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
