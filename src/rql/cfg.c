/*
 * cfg.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <limits.h>
#include <unistd.h>
//#include <sys/resource.h>
#include <sys/socket.h>
#include <rql/cfg.h>
#include <rql/sock.h>
#include <util/cfgparser.h>
#include <util/logger.h>
#include <util/strx.h>


static int rql__cfg_read_rql_path(
        cfgparser_t * parser,
        const char * cfg_file,
        char * rql_path);
static int rql__cfg_read_address_port(
        cfgparser_t * parser,
        const char * cfg_file,
        const char * option_name,
        char * address_pt,
        uint16_t * port_pt);
static void rql__cfg_read_port(
        cfgparser_t * parser,
        const char * cfg_file,
        const char * option_name,
        uint16_t * port);
static void rql__cfg_read_ip_support(
        cfgparser_t * parser,
        const char * cfg_file,
        uint8_t * ip_support);


rql_cfg_t * rql_cfg_create(void)
{
    rql_cfg_t * cfg = (rql_cfg_t *) malloc(sizeof(rql_cfg_t));
    if (!cfg) return NULL;

    /* set defaults */
    cfg->listen_client_port = 9200;
    cfg->listen_backend_port = 9220;
    cfg->ip_support = AF_UNSPEC;
    strcpy(cfg->node_address, "localhost");
    strcpy(cfg->rql_path, "/var/lib/rql/");

    return cfg;
}

void rql_cfg_destroy(rql_cfg_t * cfg)
{
    free(cfg);
}

int rql_cfg_parse(rql_cfg_t * cfg, const char * cfg_file)
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
        printf("cannot not read '%s': %s\n", cfg_file, cfgparser_errmsg(rc));
        rc = -1;
        goto exit_parse;
    }

    if ((rc = rql__cfg_read_address_port(
                parser,
                cfg_file,
                "node_name",
                cfg->node_address,
                &cfg->listen_backend_port)) ||
        (rc = rql__cfg_read_rql_path(
                parser,
                cfg_file,
                cfg->rql_path))) goto exit_parse;

    rql__cfg_read_port(
            parser,
            cfg_file,
            "listen_client_port",
            &cfg->listen_client_port);
    rql__cfg_read_ip_support(parser, cfg_file, &cfg->ip_support);

exit_parse:
    cfgparser_destroy(parser);

    return rc;
}

static void rql__cfg_read_port(
        cfgparser_t * parser,
        const char * cfg_file,
        const char * option_name,
        uint16_t * port)
{
    const int min_ = 1;
    const int max_ = 65535;

    cfgparser_option_t * option;
    cfgparser_return_t rc;
    rc = cfgparser_get_option(
                &option,
                parser,
                "rql",
                option_name);

    if (rc != CFGPARSER_SUCCESS)
    {
        log_warning(
                "Error reading '%s' in '%s': %s. "
                "(using default value: '%u')",
                option_name,
                cfg_file,
                cfgparser_errmsg(rc),
                *port);
    }
    else if (option->tp != CFGPARSER_TP_INTEGER)
    {
        log_warning(
                "Error reading '%s' in '%s': %s. "
                "(using default value: '%u')",
                option_name,
                cfg_file,
                "expecting an integer value",
                *port);
    }
    else if (option->val->integer < min_ || option->val->integer > max_)
    {
        log_warning(
                "Error reading '%s' in '%s': "
                "value should be between %d and %d but got %d. "
                "(using default value: '%u')",
                option_name,
                cfg_file,
                min_,
                max_,
                option->val->integer,
                *port);
    }
    else
    {
        *port = option->val->integer;
    }
}

static void rql__cfg_read_ip_support(
        cfgparser_t * parser,
        const char * cfg_file,
        uint8_t * ip_support)
{
    cfgparser_option_t * option;
    cfgparser_return_t rc;
    rc = cfgparser_get_option(
                &option,
                parser,
                "rql",
                "ip_support");
    if (rc != CFGPARSER_SUCCESS)
    {
        log_warning(
                "Error reading '%s' in '%s': %s. "
                "(using default value: '%s')",
                "ip_support",
                cfg_file,
                cfgparser_errmsg(rc),
                rql_sock_ip_support_str(*ip_support));
    }
    else if (option->tp != CFGPARSER_TP_STRING)
    {
        log_warning(
                "Error reading '%s' in '%s': %s. "
                "(using default value: '%s')",
                "ip_support",
                cfg_file,
                "expecting a string value",
                rql_sock_ip_support_str(*ip_support));
    }
    else
    {
        if (strcmp(option->val->string, "ALL") == 0)
        {
            *ip_support = AF_UNSPEC;
        }
        else if (strcmp(option->val->string, "IPV4ONLY") == 0)
        {
            *ip_support = AF_INET;
        }
        else if (strcmp(option->val->string, "IPV6ONLY") == 0)
        {
            *ip_support = AF_INET6;
        }
        else
        {
            log_warning(
                    "Error reading '%s' in '%s': "
                    "error: expecting ALL, IPV4ONLY or IPV6ONLY but got '%s'. "
                    "(using default value: '%s')",
                    "ip_support",
                    cfg_file,
                    option->val->string,
                    rql_sock_ip_support_str(*ip_support));
        }
    }
}

static int rql__cfg_read_rql_path(
        cfgparser_t * parser,
        const char * cfg_file,
        char * rql_path)
{
    cfgparser_option_t * option;
    cfgparser_return_t rc;
    size_t len;
    rc = cfgparser_get_option(
                &option,
                parser,
                "rql",
                "rql_path");
    if (rc != CFGPARSER_SUCCESS)
    {
        log_warning(
                "Error reading '%s' in '%s': %s. "
                "(using default value: '%s')",
                "rql_path",
                cfg_file,
                cfgparser_errmsg(rc),
                rql_path);
    }
    else if (option->tp != CFGPARSER_TP_STRING)
    {
        log_warning(
                "Error reading '%s' in '%s': %s. "
                "(using default value: '%s')",
                "rql_path",
                cfg_file,
                "error: expecting a string value",
                rql_path);
    }

    if (strlen(option->val->string) >= RQL_CFG_PATH_MAX -2 ||
        realpath(option->val->string, rql_path) == NULL)
    {
        printf( "error: cannot find rql path: %s\n", option->val->string);
        return -1;
    }

    len = strlen(rql_path);

    if (len == RQL_CFG_PATH_MAX - 2)
    {
        log_warning(
                "Default database path exceeds %d characters, please "
                "check your configuration file: %s",
                RQL_CFG_PATH_MAX - 3,
                cfg_file);
    }

    /* add trailing slash (/) if its not already there */
    if (rql_path[len - 1] != '/')
    {
        rql_path[len] = '/';
    }

    return 0;
}


/*
 * Note that address_pt must have a size of at least SIRI_CFG_MAX_LEN_ADDRESS.
 */
static int rql__cfg_read_address_port(
        cfgparser_t * parser,
        const char * cfg_file,
        const char * option_name,
        char * address_pt,
        uint16_t * port_pt)
{
    char * port;
    char * address;
    char hostname[RQL_CFG_ADDR_MAX];
    cfgparser_option_t * option;
    cfgparser_return_t rc;
    int test_port;

    if (gethostname(hostname, RQL_CFG_ADDR_MAX))
    {
        log_debug(
                "Unable to read the systems host name. Since its only purpose "
                "is to apply this in the configuration file this might not be "
                "any problem. (using 'localhost' as fallback)");
        strcpy(hostname, "localhost");
    }

    rc = cfgparser_get_option(
                &option,
                parser,
                "rql",
                option_name);
    if (rc != CFGPARSER_SUCCESS)
    {
        printf("error reading '%s' in '%s': %s\n",
                option_name,
                cfg_file,
                cfgparser_errmsg(rc));
        return -1;
    }

    if (option->tp != CFGPARSER_TP_STRING)
    {
        printf("error reading '%s' in '%s': %s\n",
                option_name,
                cfg_file,
                "expecting a string value");
        return -1;
    }

    if (*option->val->string == '[')
    {
        /* an IPv6 address... */
        for (port = address = option->val->string + 1; *port; port++)
        {
            if (*port == ']')
            {
                *port = 0;
                port++;
                break;
            }
        }
    }
    else
    {
        port = address = option->val->string;
    }

    for (; *port; port++)
    {
        if (*port == ':')
        {
            *port = 0;
            port++;
            break;
        }
    }

    if (    !strlen(address) ||
            strlen(address) >= RQL_CFG_ADDR_MAX ||
            !strx_is_int(port) ||
            strcpy(address_pt, address) == NULL ||
            strx_replace_str(
                    address_pt,
                    "%HOSTNAME",
                    hostname,
                    RQL_CFG_ADDR_MAX))
    {
        printf("error reading '%s' in '%s': "
                "got an unexpected value '%s:%s'\n",
                option_name,
                cfg_file,
                address,
                port);
        return -1;
    }
    else
    {
        test_port = atoi(port);

        if (test_port < 1 || test_port > 65535)
        {
            printf("error reading '%s' in '%s': "
                    "port should be between 1 and 65535, got '%d'\n",
                    option_name,
                    cfg_file,
                    test_port);
            return -1;
        }
        else
        {
            *port_pt = (uint16_t) test_port;
        }

        log_debug("Read '%s' from configuration: %s:%d",
                option_name,
                address_pt,
                *port_pt);
    }

    return 0;
}
