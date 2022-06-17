/*
 * ti/evars.c
 */
#include <math.h>
#include <ti.h>
#include <ti/evars.h>
#include <util/fx.h>

static void evars__bool(const char * evar, _Bool * b)
{
    char * u8str = getenv(evar);
    if (!u8str)
        return;

    *b = (_Bool) strtoul(u8str, NULL, 10);
}

static void evars__u8(const char * evar, uint8_t * u8)
{
    char * u8str = getenv(evar);
    if (!u8str)
        return;

    *u8 = (uint8_t) strtoul(u8str, NULL, 10);
}

static void evars__u16(const char * evar, uint16_t * u16)
{
    char * u16str = getenv(evar);
    if (!u16str)
        return;

    *u16 = (uint16_t) strtoul(u16str, NULL, 10);
}

static void evars__sizet(const char * evar, size_t * sz)
{
    char * sizetstr = getenv(evar);
    if (!sizetstr)
        return;

    *sz = (size_t) strtoull(sizetstr, NULL, 10);
}

static void evars__abs_double(const char * evar, double * d)
{
    char * doublestr = getenv(evar);
    if (!doublestr)
        return;

    *d = fabs(strtod(doublestr, NULL));
}

static void evars__str(const char * evar, char ** straddr)
{
    char * str = getenv(evar);
    if (!str || !(str = strdup(str)))
        return;

    free(*straddr);
    *straddr = str;
}

static void evars__ip_support(const char * evar, int * ip_support)
{
    char * str = getenv(evar);
    (void) ti_tcp_ip_support_int(str, ip_support);
}

void ti_evars_parse(void)
{
    evars__u16(
            "THINGSDB_LISTEN_CLIENT_PORT",
            &ti.cfg->client_port);
    evars__str(
            "THINGSDB_BIND_CLIENT_ADDR",
            &ti.cfg->bind_client_addr);
    evars__str(
            "THINGSDB_NODE_NAME",
            &ti.cfg->node_name);
    evars__u16(
            "THINGSDB_LISTEN_NODE_PORT",
            &ti.cfg->node_port);
    evars__str(
            "THINGSDB_BIND_NODE_ADDR",
            &ti.cfg->bind_node_addr);
    evars__ip_support(
            "THINGSDB_IP_SUPPORT",
            &ti.cfg->ip_support);
    evars__str(
            "THINGSDB_STORAGE_PATH",
            &ti.cfg->storage_path);
    evars__str(
            "THINGSDB_MODULES_PATH",
            &ti.cfg->modules_path);
    evars__bool(
            "THINGSDB_WAIT_FOR_MODULES",
            &ti.cfg->wait_for_modules);
    evars__str(
            "THINGSDB_PYTHON_INTERPRETER",
            &ti.cfg->python_interpreter);
    evars__str(
            "THINGSDB_PIPE_CLIENT_NAME",
            &ti.cfg->pipe_client_name);
    evars__sizet(
            "THINGSDB_THRESHOLD_FULL_STORAGE",
            &ti.cfg->threshold_full_storage);
    evars__sizet(
            "THINGSDB_RESULT_SIZE_LIMIT",
            &ti.cfg->result_size_limit);
    evars__sizet(
            "THINGSDB_THRESHOLD_QUERY_CACHE",
            &ti.cfg->threshold_query_cache);
    evars__sizet(
            "THINGSDB_CACHE_EXPIRATION_TIME",
            &ti.cfg->cache_expiration_time);
    evars__u16(
            "THINGSDB_HTTP_STATUS_PORT",
            &ti.cfg->http_status_port);
    evars__u16(
            "THINGSDB_HTTP_API_PORT",
            &ti.cfg->http_api_port);
    evars__u8(
            "THINGSDB_ZONE",
            &ti.cfg->zone);
    evars__abs_double(
            "THINGSDB_QUERY_DURATION_WARN",
            &ti.cfg->query_duration_warn);
    evars__abs_double(
            "THINGSDB_QUERY_DURATION_ERROR",
            &ti.cfg->query_duration_error);
    evars__str(
            "THINGSDB_GCLOUD_KEY_FILE",
            &ti.cfg->gcloud_key_file);
    evars__str(
            "THINGSDB_SSL_CERT_FILE",
            &ti.cfg->ssl_cert_file);
    evars__str(
            "THINGSDB_SSL_KEY_FILE",
            &ti.cfg->ssl_key_file);

}
