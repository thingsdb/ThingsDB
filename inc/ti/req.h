/*
 * req.h
 */
#ifndef TI_REQ_H_
#define TI_REQ_H_

typedef struct ti_req_s ti_req_t;

#include <uv.h>
#include <ti/node.h>
#include <ti/pkg.h>
#include <util/ex.h>

typedef void (*ti_req_cb)(ti_req_t * req, ex_e status);

int ti_req(
        ti_node_t * node,
        ti_pkg_t * pkg,
        uint32_t timeout,
        void * data,
        ti_req_cb cb);
void ti_req_destroy(ti_req_t * req);
void ti_req_cancel(ti_req_t * req);

struct ti_req_s
{
    uint16_t id;
    ti_node_t * node;
    ti_pkg_t * pkg_req;
    ti_pkg_t * pkg_res;
    void * data;
    uv_timer_t timer;
    ti_req_cb cb_;
};

#endif /* TI_REQ_H_ */
