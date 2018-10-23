/*
 * req.h
 */
#ifndef TI_REQ_H_
#define TI_REQ_H_

typedef struct ti_req_s ti_req_t;

#include <uv.h>
#include <ti/stream.h>
#include <ti/pkg.h>
#include <ti/ex.h>

typedef void (*ti_req_cb)(ti_req_t * req, ex_e status);

ti_req_t * ti_req_create(
        ti_stream_t * stream,
        ti_pkg_t * pkg,
        uint32_t timeout,
        ti_req_cb cb,
        void * data);
void ti_req_destroy(ti_req_t * req);
void ti_req_cancel(ti_req_t * req);

struct ti_req_s
{
    ti_stream_t * stream;
    ti_pkg_t * pkg_req;
    ti_pkg_t * pkg_res;
    void * data;
    uv_timer_t * timer;
    ti_req_cb cb_;
};

#endif /* TI_REQ_H_ */
