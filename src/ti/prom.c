/*
 * prom.c
 */
#include <stdlib.h>
#include <ti/prom.h>
ti_prom_t * ti_prom_new(size_t sz, ti_prom_cb cb, void * data)
{
    ti_prom_t * prom = malloc(sizeof(ti_prom_t) + sz * sizeof(ti_prom_res_t));
    if (!prom)
        return NULL;

    prom->cb_ = cb;
    prom->data = data;
    prom->sz = sz;
    prom->n = 0;

    return prom;
}

void ti_prom_go(ti_prom_t * prom)
{
    if (prom->n == prom->sz)
        prom->cb_(prom);
}

void ti_prom_req_cb(ti_req_t * req, ex_enum status)
{
    ti_prom_t * prom = req->data;
    ti_prom_res_t * res = &prom->res[prom->n++];
    res->tp = TI_PROM_VIA_REQ;
    res->status = status;
    res->via.req = req;
    ti_prom_go(prom);
}

void ti_prom_async_done(ti_prom_t * prom, uv_async_t * async, ex_enum status)
{
    ti_prom_res_t * res = &prom->res[prom->n++];
    res->tp = TI_PROM_VIA_ASYNC;
    res->status = status;
    res->via.async = async;
    ti_prom_go(prom);
}
