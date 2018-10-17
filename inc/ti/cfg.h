/*
 * cfg.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef TI_CFG_H_
#define TI_CFG_H_

#define TI_CFG_PATH_MAX 4096
#define TI_CFG_ADDR_MAX 256

typedef struct ti_cfg_s  ti_cfg_t;

#include <stdint.h>

ti_cfg_t * ti_cfg_new(void);
int ti_cfg_parse(ti_cfg_t * cfg, const char * cfg_file);

struct ti_cfg_s
{
    uint16_t client_port;
    uint16_t port;  /* back-end port */
    uint8_t ip_support;
    char addr[TI_CFG_ADDR_MAX];
    char ti_path[TI_CFG_PATH_MAX];
};

#endif /* TI_CFG_H_ */
