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

typedef void (*rql_write_cb)(rql_write_t * req, int status);

int rql_write(
        rql_sock_t * sock,
        rql_pkg_t * pkg,
        void * data,
        rql_write_cb cb);

struct rql_write_s
{
    rql_sock_t * sock;
    rql_pkg_t * pkg;
    void * data;
    rql_write_cb cb_;
    uv_write_t req_;
};

#endif /* RQL_WRITE_H_ */
