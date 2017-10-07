/*
 * args.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_ARGS_H_
#define RQL_ARGS_H_

typedef struct rql_args_s  rql_args_t;

#include <util/argparse.h>

rql_args_t * rql_args_new(void);
int rql_args_parse(rql_args_t * args, int argc, char *argv[]);

struct rql_args_s
{
    /* true/false props */
    int32_t version;
    int32_t log_colorized;
    int32_t init;

    /* string props */
    char config[ARGPARSE_MAX_LEN_ARG];
    char log_level[ARGPARSE_MAX_LEN_ARG];
};

#endif /* RQL_ARGS_H_ */


