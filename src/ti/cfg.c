/*
 * ti/cfg.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <ti/cfg.h>
#include <ti/stream.h>
#include <util/cfgparser.h>
#include <util/logger.h>
#include <util/strx.h>
#include <ti/tcp.h>
#include <ti.h>
#include <util/fx.h>

static ti_cfg_t * cfg;
static const char * ti__cfg_section = "thingsdb";

static int ti__cfg_storage_path(cfgparser_t * parser, const char * cfg_file);
static void ti__cfg_port(
        cfgparser_t * parser,
        const char * cfg_file,
        const char * option_name,
        uint16_t * port);
static void ti__cfg_zone(
        cfgparser_t * parser,
        const char * cfg_file,
        uint8_t * zone);
static void ti__cfg_ip_support(cfgparser_t * parser, const char * cfg_file);
static int ti__cfg_str(
        cfgparser_t * parser,
        const char * cfg_file,
        const char * option_name,
        char ** str);
static void ti__cfg_threshold_full_storage(
        cfgparser_t * parser,
        const char * cfg_file);

int ti_cfg_create(void)
{
    cfg = malloc(sizeof(ti_cfg_t));
    if (!cfg)
        return -1;

    /* set defaults */
    cfg->client_port = TI_DEFAULT_CLIENT_PORT;
    cfg->node_port = TI_DEFAULT_NODE_PORT;
    cfg->http_status_port = TI_DEFAULT_HTTP_STATUS_PORT;
    cfg->threshold_full_storage = TI_DEFAULT_THRESHOLD_FULL_STORAGE;
    cfg->ip_support = AF_UNSPEC;
    cfg->bind_client_addr = strdup("127.0.0.1");
    cfg->bind_node_addr = strdup("127.0.0.1");
    cfg->storage_path = strdup("/var/lib/thingsdb/");
    cfg->pipe_client_name = NULL;
    cfg->zone = 0;

    if (!cfg->bind_client_addr || !cfg->bind_node_addr || !cfg->storage_path)
        ti_cfg_destroy();

    ti()->cfg = cfg;
    return 0;
}

void ti_cfg_destroy(void)
{
    if (!cfg)
        return;
    free(cfg->bind_client_addr);
    free(cfg->bind_node_addr);
    free(cfg->pipe_client_name);
    free(cfg->storage_path);
    free(cfg);
    cfg = ti()->cfg = NULL;
}

int ti_cfg_parse(const char * cfg_file)
{
    int rc;
    cfgparser_t * parser = cfgparser_create();
    if (!parser)
    {
        printf("allocation error\n");
        return -1;
    }

    rc = cfgparser_read(parser, cfg_file);
    if (rc != CFGPARSER_SUCCESS)
    {
        /* we could choose to continue with defaults but this is probably
         * not what users want so lets quit.
         */
        printf("cannot not read `%s` (%s)\n", cfg_file, cfgparser_errmsg(rc));
        rc = -1;
        goto exit_parse;
    }

    if (    (rc = ti__cfg_storage_path(parser, cfg_file)) ||
            (rc = ti__cfg_str(
                    parser,
                    cfg_file,
                    "bind_client_addr",
                    &cfg->bind_client_addr)) ||
            (rc = ti__cfg_str(
                    parser,
                    cfg_file,
                    "bind_node_addr",
                    &cfg->bind_node_addr)) ||
            (rc = ti__cfg_str(
                    parser,
                    cfg_file,
                    "pipe_client_name",
                    &cfg->pipe_client_name)))
        goto exit_parse;

    ti__cfg_port(parser, cfg_file, "listen_client_port", &cfg->client_port);
    ti__cfg_port(parser, cfg_file, "listen_node_port", &cfg->node_port);
    ti__cfg_port(parser, cfg_file, "http_status_port", &cfg->http_status_port);
    ti__cfg_zone(parser, cfg_file, &cfg->zone);
    ti__cfg_ip_support(parser, cfg_file);
    ti__cfg_threshold_full_storage(parser, cfg_file);

exit_parse:
    cfgparser_destroy(parser);
    return rc;
}

static int ti__cfg_storage_path(cfgparser_t * parser, const char * cfg_file)
{
    const char * option_name = "storage_path";
    char * storage_path;
    cfgparser_option_t * option;
    cfgparser_return_t rc;
    size_t len;
    rc = cfgparser_get_option(&option, parser, ti__cfg_section, option_name);
    if (rc != CFGPARSER_SUCCESS)
    {
        log_warning(
                "missing `%s` in `%s` (%s), "
                "using default value `%s`",
                option_name,
                cfg_file,
                cfgparser_errmsg(rc),
                cfg->storage_path);
        storage_path = cfg->storage_path;
    }
    else if (option->tp != CFGPARSER_TP_STRING)
    {
        log_warning(
                "error reading `%s` in `%s` (%s), "
                "using default value `%s`",
                option_name,
                cfg_file,
                "expecting a string value",
                cfg->storage_path);
        storage_path = cfg->storage_path;
    }
    else
    {
        free(cfg->storage_path);
        storage_path = option->val->string;
    }

    if (!fx_is_dir(storage_path) && mkdir(storage_path, 0700))
        log_error("cannot create directory `%s` (%s)",
                storage_path, strerror(errno));

    cfg->storage_path = realpath(storage_path, NULL);

    if (!cfg->storage_path)
    {
        printf("cannot find storage path `%s`\n", storage_path);
        return -1;
    }

    /* add trailing slash (/) if its not already there */
    len = strlen(cfg->storage_path);
    if (cfg->storage_path[len - 1] != '/')
    {
        char * tmp = malloc(len + 2);
        if (!tmp)
        {
            printf("allocation error\n");
            return -1;
        }
        memcpy(tmp, cfg->storage_path, len);
        tmp[len] = '/';
        tmp[len + 1] = '\0';
        free(cfg->storage_path);
        cfg->storage_path = tmp;
    }

    return 0;
}

static void ti__cfg_port(
        cfgparser_t * parser,
        const char * cfg_file,
        const char * option_name,
        uint16_t * port)
{
    const int min_ = 0;
    const int max_ = 65535;

    cfgparser_option_t * option;
    cfgparser_return_t rc;
    rc = cfgparser_get_option(&option, parser, ti__cfg_section, option_name);

    if (rc != CFGPARSER_SUCCESS)
    {
        log_debug(
                "missing `%s` in `%s` (%s), "
                "using default value %u",
                option_name,
                cfg_file,
                cfgparser_errmsg(rc),
                *port);
        return;
    }
    if (    option->tp != CFGPARSER_TP_INTEGER ||
            option->val->integer < min_ ||
            option->val->integer > max_)
    {
        log_warning(
                "error reading `%s` in `%s` "
                "(expecting a value between %d and %d), "
                "using default value %u",
                option_name,
                cfg_file,
                min_,
                max_,
                *port);
        return;
    }

    *port = (uint16_t) option->val->integer;
}

static void ti__cfg_zone(
        cfgparser_t * parser,
        const char * cfg_file,
        uint8_t * zone)
{
    const int min_ = 0;
    const int max_ = 255;

    cfgparser_option_t * option;
    cfgparser_return_t rc;
    rc = cfgparser_get_option(&option, parser, ti__cfg_section, "zone");

    if (rc != CFGPARSER_SUCCESS)
    {
        log_debug(
                "missing `zone` in `%s` (%s), "
                "using default value %u",
                cfg_file,
                cfgparser_errmsg(rc),
                *zone);
        return;
    }
    if (    option->tp != CFGPARSER_TP_INTEGER ||
            option->val->integer < min_ ||
            option->val->integer > max_)
    {
        log_warning(
                "error reading `zone` in `%s` "
                "(expecting a value between %d and %d), "
                "using default value %u",
                cfg_file,
                min_,
                max_,
                *zone);
        return;
    }

    *zone = (uint8_t) option->val->integer;
}


static void ti__cfg_ip_support(cfgparser_t * parser, const char * cfg_file)
{
    const char * option_name = "ip_support";
    cfgparser_option_t * option;
    cfgparser_return_t rc;
    rc = cfgparser_get_option(&option, parser, ti__cfg_section, option_name);
    if (rc != CFGPARSER_SUCCESS)
    {
        log_warning(
                "missing `%s` in `%s` (%s), "
                "using default value `%s`",
                option_name,
                cfg_file,
                cfgparser_errmsg(rc),
                ti_tcp_ip_support_str(cfg->ip_support));
        return;
    }

    if (option->tp != CFGPARSER_TP_STRING)
    {
        log_warning(
                "error reading `%s` in `%s` (%s), "
                "using default value `%s`",
                option_name,
                cfg_file,
                "expecting a string value",
                ti_tcp_ip_support_str(cfg->ip_support));
        return;
    }

    if (strcmp(option->val->string, "ALL") == 0)
    {
        cfg->ip_support = AF_UNSPEC;
        return;
    }

    if (strcmp(option->val->string, "IPV4ONLY") == 0)
    {
        cfg->ip_support = AF_INET;
        return;
    }

    if (strcmp(option->val->string, "IPV6ONLY") == 0)
    {
        cfg->ip_support = AF_INET6;
        return;
    }

    log_warning(
            "error reading `%s` in `%s` "
            "(expecting ALL, IPV4ONLY or IPV6ONLY but got `%s`), "
            "using default value `%s`",
            "ip_support",
            cfg_file,
            option->val->string,
            ti_tcp_ip_support_str(cfg->ip_support));
}

static int ti__cfg_str(
        cfgparser_t * parser,
        const char * cfg_file,
        const char * option_name,
        char ** str)
{
    cfgparser_option_t * option;
    cfgparser_return_t rc;
    rc = cfgparser_get_option(&option, parser, ti__cfg_section, option_name);
    if (rc != CFGPARSER_SUCCESS)
    {
        if (*str)
            log_warning(
                    "missing `%s` in `%s` (%s), "
                    "using default value `%s`",
                    option_name,
                    cfg_file,
                    cfgparser_errmsg(rc),
                    *str);
        return 0;
    }
    if (option->tp != CFGPARSER_TP_STRING)
    {
        log_warning(
                "error reading `%s` in `%s` (expecting a string value), "
                "Using default value `%s`",
                option_name,
                cfg_file,
                *str ? *str : "disabled");
        return 0;
    }
    if (*option->val->string == '\0')
    {
        if (*str)
            log_warning(
                    "missing `%s` in `%s` (%s), "
                    "using default value `%s`",
                    option_name,
                    cfg_file,
                    cfgparser_errmsg(rc),
                    *str);
        return 0;
    }

    free(*str);
    *str = strdup(option->val->string);
    return -(!*str);
}

static void ti__cfg_threshold_full_storage(
        cfgparser_t * parser,
        const char * cfg_file)
{
    const char * option_name = "threshold_full_storage";

    cfgparser_option_t * option;
    cfgparser_return_t rc;
    rc = cfgparser_get_option(&option, parser, ti__cfg_section, option_name);

    if (rc != CFGPARSER_SUCCESS)
    {
        log_debug(
                "missing `%s` in `%s` (%s), "
                "using default value %zu",
                option_name,
                cfg_file,
                cfgparser_errmsg(rc),
                cfg->threshold_full_storage);
        return;
    }
    if (    option->tp != CFGPARSER_TP_INTEGER ||
            option->val->integer < 0)
    {
        log_warning(
                "error reading `%s` in `%s` "
                "(expecting an integer value greater than, or equal to 0), "
                "using default value %zu",
                option_name,
                cfg_file,
                cfg->threshold_full_storage);
        return;
    }

    cfg->threshold_full_storage = (size_t) option->val->integer;
}
