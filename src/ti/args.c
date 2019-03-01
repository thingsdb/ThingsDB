/*
 * args.c
 */
#include <util/argparse.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ti/args.h>
#include <ti.h>
#include <util/strx.h>

#define ARGS__NO_ZONE INT32_MIN

#ifndef NDEBUG
#define DEFAULT_LOG_LEVEL "debug"
#else
#define DEFAULT_LOG_LEVEL "warning"
#endif

static ti_args_t * args;

int ti_args_create(void)
{
    args = calloc(1, sizeof(ti_args_t));
    if (!args)
        return -1;
    args->version = 0;
    strcpy(args->config, "");
    strcpy(args->log_level, "");
    args->zone = ARGS__NO_ZONE;
    args->log_colorized = 0;
    args->init = 0;
    ti()->args = args;

    return 0;
}

void ti_args_destroy(void)
{
    free(args);
    args = ti()->args = NULL;
}

uint8_t ti_args_get_zone(void)
{
    return args->zone == ARGS__NO_ZONE ? 0 : args->zone;
}

_Bool ti_args_has_zone(void)
{
    return args->zone != ARGS__NO_ZONE;
}

int ti_args_parse(int argc, char *argv[])
{
    int rc;
    argparse_t * parser = argparse_create();
    if (!parser)
        return -1;

    argparse_argument_t config_ = {
            name: "config",
            shortcut: 'c',
            help: "define which ThingsDB configuration file to use",
            action: ARGPARSE_STORE_STRING,
            default_int32_t: 0,
            pt_value_int32_t: NULL,
            str_default: "/etc/thingsdb/thingsdb.conf",
            str_value: args->config,
            choices: NULL
    };

    argparse_argument_t init_ = {
            name: "init",
            shortcut: 0,
            help: "initialize a new ThingsDB store",
            action: ARGPARSE_STORE_TRUE,
            default_int32_t: 0,
            pt_value_int32_t: &args->init,
            str_default: NULL,
            str_value: NULL,
            choices: NULL,
    };

    argparse_argument_t secret_ = {
            name: "secret",
            shortcut: 0,
            help: "set one time secret for nodes to connect",
            action: ARGPARSE_STORE_STRING,
            default_int32_t: 0,
            pt_value_int32_t: NULL,
            str_default: "",
            str_value: args->secret,
            choices: NULL,
    };

    argparse_argument_t zone_ = {
            name: "zone",
            shortcut: 0,
            help: "set the node zone, can be overwritten at runtime using set_zone(...)",
            action: ARGPARSE_STORE_INT,
            pt_value_int32_t: &args->zone,
            str_default: NULL,
            str_value: NULL,
            choices: NULL,
    };

    argparse_argument_t version_ = {
            name: "version",
            shortcut: 'v',
            help: "show version information and exit",
            action: ARGPARSE_STORE_TRUE,
            default_int32_t: 0,
            pt_value_int32_t: &args->version,
            str_default: NULL,
            str_value: NULL,
            choices: NULL
    };

    argparse_argument_t log_level_ = {
            name: "log-level",
            shortcut: 'l',
            help: "set initial log level",
            action: ARGPARSE_STORE_STR_CHOICE,
            default_int32_t: 0,
            pt_value_int32_t: NULL,
            str_default: DEFAULT_LOG_LEVEL,
            str_value: args->log_level,
            choices: "debug,info,warning,error,critical"
    };

    argparse_argument_t log_colorized_ = {
            name: "log-colorized",
            shortcut: 0,
            help: "use colorized logging",
            action: ARGPARSE_STORE_TRUE,
            default_int32_t: 0,
            pt_value_int32_t: &args->log_colorized,
            str_default: NULL,
            str_value: NULL,
            choices: NULL,
    };

    if (    argparse_add_argument(parser, &config_) ||
            argparse_add_argument(parser, &init_) ||
            argparse_add_argument(parser, &secret_) ||
            argparse_add_argument(parser, &zone_) ||
            argparse_add_argument(parser, &version_) ||
            argparse_add_argument(parser, &log_level_) ||
            argparse_add_argument(parser, &log_colorized_))
        return -1;

    /* this will parse and free the parser from memory */
    rc = argparse_parse(parser, argc, argv);

    if (rc == 0)
    {
        if (!args->init)
        {
            if (*args->secret && !strx_is_graph(args->secret))
            {
                printf("secret should only contain graphic characters\n");
                rc = -1;
            }
        }

        if (args->init && *args->secret)
        {
            printf("--secret cannot be used together with --init\n");
            rc = -1;
        }

        if (args->zone != ARGS__NO_ZONE && (args->zone < 0 || args->zone > 255))
        {
            printf("zone must be a value between 0 and 255\n");
            rc = -1;
        }
    }

    argparse_destroy(parser);
    return rc;
}
