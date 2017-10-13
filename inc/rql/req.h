/*
 * req.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_REQ_H_
#define RQL_REQ_H_

typedef struct rql_req_s rql_req_t;

#include <uv.h>
#include <rql/node.h>
#include <rql/pkg.h>
#include <util/ex.h>

typedef void (*rql_req_cb)(rql_req_t * req, ex_e status);

int rql_req(
        rql_node_t * node,
        rql_pkg_t * pkg,
        uint32_t timeout,
        void * data,
        rql_req_cb cb);
void rql_req_destroy(rql_req_t * req);
void rql_req_cancel(rql_req_t * req);

struct rql_req_s
{
    uint16_t id;
    rql_node_t * node;
    rql_pkg_t * pkg_req;
    rql_pkg_t * pkg_res;
    void * data;
    uv_timer_t timer;
    rql_req_cb cb_;
};

#endif /* RQL_REQ_H_ */
