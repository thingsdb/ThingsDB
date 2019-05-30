/*
 * ti/cfg.h
 */
#ifndef TI_CFG_H_
#define TI_CFG_H_

typedef struct ti_cfg_s  ti_cfg_t;

#include <stdint.h>

int ti_cfg_create(void);
void ti_cfg_destroy(void);
int ti_cfg_parse(const char * cfg_file);

struct ti_cfg_s
{
    uint16_t client_port;
    uint16_t node_port;
    uint16_t http_status_port;
    size_t threshold_full_storage;      /* if the number of events changes
                                           stored on disk is equal or greater
                                           than this threshold, then a full-
                                           instead of incremental store will be
                                           performed
                                        */
    int ip_support;                     /* AF_UNSPEC / AF_INET / AF_INET6 */
    char * bind_client_addr;
    char * bind_node_addr;
    char * pipe_client_name;
    char * storage_path;
};

#endif /* TI_CFG_H_ */
