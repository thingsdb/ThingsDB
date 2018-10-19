/*
 * args.h
 */
#ifndef THINGSDB_ARGS_H_
#define THINGSDB_ARGS_H_

typedef struct thingsdb_args_s  thingsdb_args_t;

#include <util/argparse.h>

int thingsdb_args_create(void);
void thingsdb_args_destroy(void);
int thingsdb_args_parse(thingsdb_args_t * args, int argc, char *argv[]);

struct thingsdb_args_s
{
    /* true/false props */
    int32_t version;
    int32_t log_colorized;
    int32_t init;

    /* string props */
    char config[ARGPARSE_MAX_LEN_ARG];
    char log_level[ARGPARSE_MAX_LEN_ARG];
    char secret[ARGPARSE_MAX_LEN_ARG];
};

#endif /* THINGSDB_ARGS_H_ */


