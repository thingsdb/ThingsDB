/*
 * qpx.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <util/qpx.h>

qp_res_t * qpx_map_get(qp_map_t * map, const char * key)
{
    for (size_t i = 0; map->n; i++)
    {
        qp_res_t * key = map->keys[i];
        if (key->tp == QP_RES_STR && strcmp(key.via->str, key) == 0)
        {
            return map->values[i];
        }
    }
    return NULL;
}
