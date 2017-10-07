/*
 * pkg.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <rql/pkg.h>


rql_pkg_t * rql_pkg_new(uint8_t tp, const unsigned char * data, uint32_t n)
{
    rql_pkg_t * pkg = (rql_pkg_t *) malloc(sizeof(rql_pkg_t) + n);
    if (!pkg) return NULL;
    pkg->id = 0;
    pkg->tp = tp;
    pkg->ntp = tp ^ 255;
    pkg->n = n;
    memcpy(pkg->data, data, n);
    return pkg;
}

