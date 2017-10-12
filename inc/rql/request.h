/*
 * request.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_REQUEST_H_
#define RQL_REQUEST_H_

typedef struct rql_request_s  rql_request_t;

#include <inttypes.h>
#include <rql/sock.h>

struct rql_request_s
{
    uint64_t ref;
    uint16_t id;
};


#endif /* RQL_REQUEST_H_ */
