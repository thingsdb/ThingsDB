/*
 * prom.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */

rql_prom_t * rql_prom_create(size_t sz, void * data, rql_prom_cb cb)
{
    rql_prom_t * prom = (rql_prom_t *) malloc(
            sizeof(rql_prom_t) + sz * sizeof(rql_prom_res_t));
    if (!prom) return -1;

    prom->n = 0;
    prom->sz = sz;
    prom->cb_ = cb;

    return prom;
}

rql_prom_t * rql_prom_grab(rql_prom_t * prom)
{
    prom->ref++;
    return prom;
}

void rql_prom_drop(rql_prom_t * prom)
{
    if (prom && !--prom->ref)
    {
        free(prom);
    }
}

void rql_prom_go(rql_prom_t * prom)
{
    if (prom->n == prom->sz)
    {
        prom->cb_(prom);
    }
}

void rql_prom_req_cb(rql_req_t * req, ex_t status)
{
    rql_prom_t * prom = (rql_prom_t *) req->data;
    rql_prom_res_t * res = &prom->data[prom->n++];
    res->tp = RQL_PROM_VIA_REQ;
    res->status = status;
    res->handle = req;
    rql_prom_go(prom);
}
