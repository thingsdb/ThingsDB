/*
 * prom.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef TI_PROM_H_
#define TI_PROM_H_

typedef enum
{
    TI_PROM_VIA_REQ,
    TI_PROM_VIA_ASYNC
} ti_prom_e;

typedef struct ti_prom_s ti_prom_t;
typedef struct ti_prom_res_s ti_prom_res_t;

#include <uv.h>
#include <ti/req.h>
#include <util/vec.h>
#include <util/ex.h>

typedef void (*ti_prom_cb)(ti_prom_t * prom);

ti_prom_t * ti_prom_new(size_t sz, void * data, ti_prom_cb cb);
void ti_prom_go(ti_prom_t * prom);
void ti_prom_req_cb(ti_req_t * req, ex_e status);
void ti_prom_async_done(ti_prom_t * prom, uv_async_t * async, ex_e status);

struct ti_prom_res_s
{
    ti_prom_e tp;
    ex_e status;
    void * handle;
};

struct ti_prom_s
{
    size_t n;
    size_t sz;
    void * data;
    ti_prom_cb cb_;
    ti_prom_res_t res[];
};

#endif /* TI_PROM_H_ */
