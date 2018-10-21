/*
 * args.h
 */
#ifndef TI_ARGS_H_
#define TI_ARGS_H_

typedef struct ti_args_s  ti_args_t;

#include <util/argparse.h>

int ti_args_create(void);
void ti_args_destroy(void);
int ti_args_parse(int argc, char *argv[]);

struct ti_args_s
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

#endif /* TI_ARGS_H_ */


