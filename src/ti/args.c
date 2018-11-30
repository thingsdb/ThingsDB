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

#define DEFAULT_REDUNDANCY 4

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
    args->redundancy = 0;
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

    argparse_argument_t redundancy_ = {
            name: "redundancy",
            shortcut: 0,
            help: "set the initial redundancy (only together with --init)",
            action: ARGPARSE_STORE_INT,
            default_int32_t: DEFAULT_REDUNDANCY,
            pt_value_int32_t: &args->redundancy,
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
            argparse_add_argument(parser, &redundancy_) ||
            argparse_add_argument(parser, &secret_) ||
            argparse_add_argument(parser, &version_) ||
            argparse_add_argument(parser, &log_level_) ||
            argparse_add_argument(parser, &log_colorized_))
        return -1;

    /* this will parse and free the parser from memory */
    rc = argparse_parse(parser, argc, argv);

    if (rc == 0)
    {
        if (!args->init && args->redundancy)
        {
            printf("redundancy can only be set together with --init\n"
                   "see "TI_DOCS"#redundancy for more info\n");
            rc = -1;
        }

        if (args->init)
        {
            if (!args->redundancy)
                args->redundancy = DEFAULT_REDUNDANCY;
            else if (args->redundancy < 1 || args->redundancy > 64)
            {
                printf("redundancy must be a value between 1 and 64\n");
                rc = -1;
            }
        }
    }

    argparse_destroy(parser);

    return rc;
}
