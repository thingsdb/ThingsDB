/*
 * ti/prom.h
 */
#ifndef TI_PROM_H_
#define TI_PROM_H_

typedef enum
{
    TI_PROM_VIA_REQ,
    TI_PROM_VIA_ASYNC
} ti_prom_enum;

typedef struct ti_prom_s ti_prom_t;
typedef struct ti_prom_res_s ti_prom_res_t;
typedef union ti_prom_via_u ti_prom_via_t;

#include <uv.h>
#include <ti/req.h>
#include <util/vec.h>
#include <ex.h>

typedef void (*ti_prom_cb)(ti_prom_t * prom);

ti_prom_t * ti_prom_new(size_t sz, ti_prom_cb cb, void * data);
static inline void ti_prom_destroy(ti_prom_t * prom);
void ti_prom_go(ti_prom_t * prom);
void ti_prom_req_cb(ti_req_t * req, ex_enum status);
void ti_prom_async_done(ti_prom_t * prom, uv_async_t * async, ex_enum status);

union ti_prom_via_u
{
    ti_req_t * req;
    uv_async_t * async;
};

struct ti_prom_res_s
{
    ti_prom_enum tp;
    ex_enum status;
    ti_prom_via_t via;
};

struct ti_prom_s
{
    size_t n;
    size_t sz;
    void * data;
    ti_prom_cb cb_;
    ti_prom_res_t res[];
};

static inline void ti_prom_destroy(ti_prom_t * prom)
{
    free(prom);
}

#endif /* TI_PROM_H_ */
