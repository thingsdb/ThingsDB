/*
 * prom.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_PROM_H_
#define RQL_PROM_H_

typedef enum
{
    RQL_PROM_VIA_REQ,
    RQL_PROM_VIA_ASYNC
} rql_prom_e;

typedef struct rql_prom_s rql_prom_t;
typedef struct rql_prom_res_s rql_prom_res_t;

#include <uv.h>
#include <rql/req.h>
#include <util/vec.h>
#include <util/ex.h>

typedef void (*rql_prom_cb)(rql_prom_t * prom);

rql_prom_t * rql_prom_new(size_t sz, void * data, rql_prom_cb cb);
//rql_prom_t * rql_prom_grab(rql_prom_t * prom);
//void rql_prom_destroy(rql_prom_t * prom);
void rql_prom_go(rql_prom_t * prom);
void rql_prom_req_cb(rql_req_t * req, ex_e status);

struct rql_prom_res_s
{
    rql_prom_e tp;
    ex_e status;
    void * handle;
};

struct rql_prom_s
{
//    int64_t ref;
    size_t n;
    size_t sz;
    void * data;
    rql_prom_cb cb_;
    rql_prom_res_t res[];
};

#endif /* RQL_PROM_H_ */
