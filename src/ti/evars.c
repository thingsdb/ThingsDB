/*
 * ti/evars.c
 */
#include <ti.h>
#include <ti/evars.h>
#include <util/fx.h>

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

static void evars__double(const char * evar, double * d)
{
    char * doublestr = getenv(evar);
    if (!doublestr)
        return;

    *d = strtod(doublestr, NULL);
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
            &ti()->cfg->client_port);
    evars__str(
            "THINGSDB_BIND_CLIENT_ADDR",
            &ti()->cfg->bind_client_addr);
    evars__str(
            "THINGSDB_NODE_NAME",
            &ti()->cfg->node_name);
    evars__u16(
            "THINGSDB_LISTEN_NODE_PORT",
            &ti()->cfg->node_port);
    evars__str(
            "THINGSDB_BIND_NODE_ADDR",
            &ti()->cfg->bind_node_addr);
    evars__ip_support(
            "THINGSDB_IP_SUPPORT",
            &ti()->cfg->ip_support);
    evars__str(
            "THINGSDB_STORAGE_PATH",
            &ti()->cfg->storage_path);
    evars__str(
            "THINGSDB_PIPE_CLIENT_NAME",
            &ti()->cfg->pipe_client_name);
    evars__sizet(
            "THINGSDB_THRESHOLD_FULL_STORAGE",
            &ti()->cfg->threshold_full_storage);
    evars__u16(
            "THINGSDB_HTTP_STATUS_PORT",
            &ti()->cfg->http_status_port);
    evars__u16(
            "THINGSDB_HTTP_API_PORT",
            &ti()->cfg->http_api_port);
    evars__u8(
            "THINGSDB_ZONE",
            &ti()->cfg->zone);
    evars__double(
            "THINGSDB_QUERY_DURATION_WARN",
            &ti()->cfg->query_duration_warn);
    evars__double(
            "THINGSDB_QUERY_DURATION_ERROR",
            &ti()->cfg->query_duration_error);
}

void ti_evars_deploy(void)
{
    size_t n;
    char * pod_id;

    if (fx_file_exist(ti()->fn) ||
        !(pod_id = getenv("THINGSDB_POD_ID")))
        return;

    n = strlen(pod_id);
    if (!n)
        return;

    if (pod_id[n-1] == '0' && (n == 1 || pod_id[n-2] == '-'))
    {
        /* this is the fist node and ThingsDB is not yet initialized */
        ti()->args->init = 1;
    }
    else if (!(*ti()->args->secret))
    {
        char * pod_secret = getenv("THINGSDB_POD_SECRET");
        pod_secret = pod_secret ? pod_secret : pod_id;

        memset(ti()->args->secret, 0, ARGPARSE_MAX_LEN_ARG);
        strncpy(ti()->args->secret, pod_secret, ARGPARSE_MAX_LEN_ARG-1);
    }
}
