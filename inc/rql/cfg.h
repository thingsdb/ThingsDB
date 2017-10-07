/*
 * cfg.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_CFG_H_
#define RQL_CFG_H_

#define RQL_CFG_PATH_MAX 4096
#define RQL_CFG_ADDR_MAX 256

typedef struct rql_cfg_s  rql_cfg_t;

#include <inttypes.h>

rql_cfg_t * rql_cfg_new(void);
int rql_cfg_parse(rql_cfg_t * cfg, const char * cfg_file);

struct rql_cfg_s
{
    uint16_t client_port;
    uint16_t port;  /* back-end port */
    uint8_t ip_support;
    char addr[RQL_CFG_ADDR_MAX];
    char rql_path[RQL_CFG_PATH_MAX];
};

#endif /* RQL_CFG_H_ */
