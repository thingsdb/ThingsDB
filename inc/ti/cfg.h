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
int ti_cfg_ensure_storage_path(void);

struct ti_cfg_s
{
    uint16_t client_port;
    uint16_t node_port;
    uint16_t http_status_port;
    uint16_t http_api_port;
    uint16_t ws_port;                   /* default 0 */
    uint8_t zone;
    uint8_t shutdown_period;            /* Wait for X seconds before shutdown;
                                          (only used with multiple nodes) */
    size_t threshold_full_storage;      /* if the number of changes
                                           stored on disk is equal or greater
                                           than this threshold, then a full-
                                           instead of incremental store will be
                                           performed.
                                        */
    size_t result_size_limit;           /* error EX_RESULT_TOO_LARGE is raised
                                           when the return size exceeds this
                                           limit.
                                        */
    size_t threshold_query_cache;       /* use query cache for queries above
                                           this threshold size.
                                        */
    size_t cache_expiration_time;       /* cached queries which are not used
                                           within the expiration time will be
                                           remove from cache. This check only
                                           takes place while in `away` mode.
                                       */
    int ip_support;                    /* AF_UNSPEC / AF_INET / AF_INET6 */
    _Bool wait_for_modules;            /* wait for modules to load before
                                          listening to nodes and clients */
    char * node_name;
    char * bind_client_addr;
    char * bind_node_addr;
    char * pipe_client_name;
    char * storage_path;                /* with trailing `/` */
    char * gcloud_key_file;
    char * modules_path;                /* path to find modules */
    char * python_interpreter;          /* python interpreter */
    char * ws_cert_file;                /* path to certificate file */
    char * ws_key_file;                 /* path to private key file */
    double query_duration_warn;
    double query_duration_error;
};

#endif /* TI_CFG_H_ */
