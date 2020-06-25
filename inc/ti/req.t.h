/*
 * ti/req.t.h
 */
#ifndef TI_REQ_T_H_
#define TI_REQ_T_H_

typedef struct ti_req_s ti_req_t;

#include <ex.h>
#include <ti/pkg.t.h>
#include <ti/stream.t.h>
#include <uv.h>

typedef void (*ti_req_cb)(ti_req_t * req, ex_enum status);

struct ti_req_s
{
    ti_stream_t * stream;
    ti_pkg_t * pkg_req;
    ti_pkg_t * pkg_res;
    void * data;
    uv_timer_t * timer;
    ti_req_cb cb_;
};

#endif /* TI_REQ_T_H_ */
