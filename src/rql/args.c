/*
 * args.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <util/argparse.h>
#include <rql/args.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define DEFAULT_LOG_FILE_MAX_SIZE 50000000
#define DEFAULT_LOG_FILE_NUM_BACKUPS 6

#ifdef DEBUG
#define DEFAULT_LOG_LEVEL "debug"
#else
#define DEFAULT_LOG_LEVEL "info"
#endif

rql_args_t * rql_args_new(void)
{
    rql_args_t * args = (rql_args_t *) malloc(sizeof(rql_args_t));
    if (!args) return NULL;
    args->version = 0;
    strcpy(args->config, "");
    strcpy(args->log_level, "");
    args->log_colorized = 0;
    return args;
}

int rql_args_parse(rql_args_t * args, int argc, char *argv[])
{
    int rc;
    argparse_t * parser = argparse_create();
    if (!parser) return -1;

    argparse_argument_t config_ = {
            name: "config",
            shortcut: 'c',
            help: "define which RQL configuration file to use",
            action: ARGPARSE_STORE_STRING,
            default_int32_t: 0,
            pt_value_int32_t: NULL,
            str_default: "/etc/rql/rql.conf",
            str_value: args->config,
            choices: NULL
    };

    argparse_argument_t init_ = {
            name: "init",
            shortcut: 0,
            help: "initialize a new RQL store",
            action: ARGPARSE_STORE_TRUE,
            default_int32_t: 0,
            pt_value_int32_t: &args->init,
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
            argparse_add_argument(parser, &version_) ||
            argparse_add_argument(parser, &log_level_) ||
            argparse_add_argument(parser, &log_colorized_)) return -1;

    /* this will parse and free the parser from memory */
    rc = argparse_parse(parser, argc, argv);

    argparse_destroy(parser);

    return rc;
}
