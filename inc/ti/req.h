/*
 * ti/req.h
 */
#ifndef TI_REQ_H_
#define TI_REQ_H_

#include <inttypes.h>
#include <ti/pkg.t.h>
#include <ti/req.t.h>
#include <ti/stream.t.h>

int ti_req_create(
        ti_stream_t * stream,
        ti_pkg_t * pkg_req,
        uint32_t timeout,
        ti_req_cb cb,
        void * data);
void ti_req_destroy(ti_req_t * req);
void ti_req_cancel(ti_req_t * req);
void ti_req_result(ti_req_t * req);

#endif /* TI_REQ_H_ */
