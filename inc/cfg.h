/*
 * cfg.h
 */
#ifndef TI_CFG_H_
#define TI_CFG_H_

typedef struct ti_cfg_s  ti_cfg_t;

#include <stdint.h>

int thingsdb_cfg_create(void);
void thingsdb_cfg_destroy(void);
int thingsdb_cfg_parse(const char * cfg_file);

struct ti_cfg_s
{
    uint16_t client_port;
    uint16_t node_port;
    int ip_support;
    _Bool pipe_support;
    char * bind_client_addr;
    char * bind_node_addr;
    char * pipe_client_name;
    char * pipe_node_name;
    char * addr;
    char * store_path;
};

#endif /* TI_CFG_H_ */
