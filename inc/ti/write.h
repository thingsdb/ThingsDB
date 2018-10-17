/*
 * write.h
 */
#ifndef TI_WRITE_H_
#define TI_WRITE_H_

typedef struct ti_write_s ti_write_t;

#include <uv.h>
#include <ti/sock.h>
#include <ti/pkg.h>
#include <util/ex.h>

typedef void (*ti_write_cb)(ti_write_t * req, ex_e status);

int ti_write(
        ti_sock_t * sock,
        ti_pkg_t * pkg,
        void * data,
        ti_write_cb cb);
void ti_write_destroy(ti_write_t * req);

struct ti_write_s
{
    ti_sock_t * sock;
    ti_pkg_t * pkg;
    void * data;
    ti_write_cb cb_;
    uv_write_t req_;
};

#endif /* TI_WRITE_H_ */
