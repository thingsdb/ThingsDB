/*
 * write.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_WRITE_H_
#define RQL_WRITE_H_

typedef struct rql_write_s rql_write_t;

#include <uv.h>
#include <rql/sock.h>
#include <rql/pkg.h>
#include <util/ex.h>

typedef void (*rql_write_cb)(rql_write_t * req, ex_e status);

int rql_write(
        rql_sock_t * sock,
        rql_pkg_t * pkg,
        void * data,
        rql_write_cb cb);
void rql_write_destroy(rql_write_t * req);

struct rql_write_s
{
    rql_sock_t * sock;
    rql_pkg_t * pkg;
    void * data;
    rql_write_cb cb_;
    uv_write_t req_;
};

#endif /* RQL_WRITE_H_ */
