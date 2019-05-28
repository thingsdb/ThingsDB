/*
 * args.h
 */
#ifndef TI_ARGS_H_
#define TI_ARGS_H_

typedef struct ti_args_s  ti_args_t;

#include <util/argparse.h>

int ti_args_create(void);
void ti_args_destroy(void);
uint8_t ti_args_get_zone(void);
_Bool ti_args_has_zone(void);
int ti_args_parse(int argc, char *argv[]);

struct ti_args_s
{
    /* true/false props */
    int32_t force;
    int32_t init;
    int32_t log_colorized;
    int32_t rebuild;
    int32_t version;

    /* integer props */
    int32_t zone;                           /* can be overwritten at runtime */

    /* string props */
    char config[ARGPARSE_MAX_LEN_ARG];
    char log_level[ARGPARSE_MAX_LEN_ARG];   /* can be overwritten at runtime */
    char secret[ARGPARSE_MAX_LEN_ARG];      /* allow only graphic characters */
};

#endif /* TI_ARGS_H_ */


