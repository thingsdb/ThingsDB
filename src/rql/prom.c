/*
 * prom.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <rql/prom.h>

rql_prom_t * rql_prom_new(size_t sz, void * data, rql_prom_cb cb)
{
    rql_prom_t * prom = (rql_prom_t *) malloc(
            sizeof(rql_prom_t) + sz * sizeof(rql_prom_res_t));
    if (!prom) return NULL;

    prom->cb_ = cb;
    prom->data = data;
    prom->sz = sz;
    prom->n = 0;

    return prom;
}

void rql_prom_go(rql_prom_t * prom)
{
    if (prom->n == prom->sz)
    {
        prom->cb_(prom);
    }
}

void rql_prom_req_cb(rql_req_t * req, ex_e status)
{
    rql_prom_t * prom = (rql_prom_t *) req->data;
    rql_prom_res_t * res = &prom->res[prom->n++];
    res->tp = RQL_PROM_VIA_REQ;
    res->status = status;
    res->handle = req;
    rql_prom_go(prom);
}

void rql_prom_async_done(rql_prom_t * prom, uv_async_t * async, ex_e status)
{
    rql_prom_res_t * res = &prom->res[prom->n++];
    res->tp = RQL_PROM_VIA_ASYNC;
    res->status = status;
    res->handle = async;
    rql_prom_go(prom);
}
